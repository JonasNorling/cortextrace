#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <fcntl.h>
#include <cmath>

#include "Registers.h"
#include "TraceEvent.h"
#include "TraceEventListener.h"
#include "TraceFileParser.h"
#include "GdbConnection.h"
#include "log.h"

#define DEFAULT_GDB "arm-none-eabi-gdb"
#define DEFAULT_GDB_TARGET "extended-remote :3333"
#define DEFAULT_CORE_FREQ 72000000UL

class Pipe {
public:
    Pipe() throw(std::runtime_error);
    ~Pipe();
    int OpenForReading();
    std::string GetName() const { return Name; }

protected:
    std::string Name;
    int Fd;
};

class CortexWatch : public lct::TraceEventListener {
public:
    CortexWatch() { }
    virtual ~CortexWatch();
    int Run(std::string gdbPath, std::string gdbTarget,
            std::string elfPath, size_t corefreq,
            const std::vector<std::string>& watch);
    void Exit();
    void OpenPipe();

    // interface TraceEventListener
    void HandleTraceEvent(const lct::TraceEvent& event);

protected:
    bool TimeToExit;
    int PipeFd;
    std::unique_ptr<Pipe> TpiuPipe;
};

static CortexWatch s_cortexWatch;

// -----------------------------------------------------------------

CortexWatch::~CortexWatch()
{
}

void CortexWatch::HandleTraceEvent(const lct::TraceEvent& event)
{
    switch (event.Type) {
    case lct::TraceEvent::TRACE_EVENT_INSTR:
        std::cout << static_cast<char>(event.Value);
        break;
    case lct::TraceEvent::TRACE_EVENT_HW: {
        const uint8_t discriminator = event.Code >> 3;
        if (discriminator == 0x2) { // PC sample
            std::cout << "PC: " << std::hex << event.Value << std::dec << std::endl;
        }
        else if ((discriminator & 0x19) == 0x08) { // PC trace
            std::cout << "PC trace: " << std::hex << event.Value << std::dec << std::endl;
        }
        else if ((discriminator & 0x18) == 0x10) { // data trace
            std::cout << "data trace: " << ((discriminator & 0x01) ? "W " : "R " )
                    << std::hex << event.Value << std::dec << std::endl;
        }
        else {
            std::cout << "HW event: " << std::hex << event.Code << ":" << event.Value << std::dec << std::endl;
        }
        break;
    }
    case lct::TraceEvent::TRACE_EVENT_OVERFLOW:
        std::cout << "Overflow" << std::endl;
        break;
    case lct::TraceEvent::TRACE_EVENT_SYNC:
        std::cout << "Sync" << std::endl;
        break;
    default:
        std::cout << "Event " << event.Type << ", "
        << event.Code << ", " << event.Value << std::endl;
    }
}

void CortexWatch::OpenPipe()
{
    PipeFd = TpiuPipe->OpenForReading();
    LOG_DEBUG("Opened pipe stream");
}

int CortexWatch::Run(std::string gdbPath, std::string gdbTarget,
        std::string elfPath, size_t corefreq,
        const std::vector<std::string>& watch)
{
    TpiuPipe.reset(new Pipe);

    lct::GdbConnection gdb;
    lct::TraceFileParser tfp(*this);
    lct::Registers regs;

    gdb.Connect(gdbPath, elfPath);
    gdb.TargetSelect(gdbTarget);
    gdb.DisableTpiu();

    const uint32_t cpuid = gdb.ReadWord(regs.CPUID);
    const uint32_t partno = (cpuid >> 4) & 0xfff;
    LOG_INFO("CPUID: %#x: %s %s%u r%up%u",
            cpuid,
            (cpuid >> 24) == 0x41 ? "ARM" : "unknown",
            (partno & 0xc30) == 0xc20 ? "Cortex-M" : "unknown",
            partno & 0x0f,
            (cpuid >> 20) & 0xf,
            cpuid & 0xf);

    const size_t numcomp = gdb.ReadWord(regs.DWT_CTRL) >> 28;
    LOG_DEBUG("%lu comparators on this chip", numcomp);

    if (watch.size() > numcomp) {
        LOG_ERROR("Too many expressions, hardware only has %lu comparators",
                numcomp);
        return 1;
    }

    if ((gdb.ReadWord(regs.ROMTPIU) & 0x3) != 0x3) {
        LOG_WARNING("No TPIU fitted, tracing will not work");
    }
    if ((gdb.ReadWord(regs.ROMDWT) & 0x3) != 0x3) {
        LOG_WARNING("No DWT fitted, tracing will not work");
    }

    LOG_INFO("CPU core has support for %s%s%s%s%s.",
            (gdb.ReadWord(regs.ROMDWT) & 0x3) == 0x3 ? "DWT " : "",
            (gdb.ReadWord(regs.ROMFPB) & 0x3) == 0x3 ? "FPB " : "",
            (gdb.ReadWord(regs.ROMITM) & 0x3) == 0x3 ? "ITM " : "",
            (gdb.ReadWord(regs.ROMTPIU) & 0x3) == 0x3 ? "TPIU " : "",
            (gdb.ReadWord(regs.ROMETM) & 0x3) == 0x3 ? "ETM " : "");

    // We need to open the pipe for reading before telling OpenOCD to open it
    // for writing. The open call will block until both ends are open, so we
    // need to do let it block "in the background" here.
    std::thread openthread([this](){ this->OpenPipe(); });

    LOG_DEBUG("Enable TPIU");
    gdb.EnableTpiu(TpiuPipe->GetName(), corefreq);

    // Clear old watches
    for (size_t comp = 0; comp < numcomp; comp++) {
        gdb.WriteWord(regs.DWT_COMP[comp], 0);
        gdb.WriteWord(regs.DWT_MASK[comp], 0);
        gdb.WriteWord(regs.DWT_FUNCTION[comp], 0);
    }

    // Set up new watches
    size_t comp = 0;
    for (auto expression : watch) {
        LOG_DEBUG("Setting watch");
        const size_t size = std::stoul(gdb.Evaluate(std::string("sizeof(") +
                expression + ")"));
        const uint32_t addr = gdb.ResolveAddress(std::string("&(") + expression + ")");

        const size_t masksize = log2(size);
        if (1 << masksize != size) {
            LOG_WARNING("Cannot watch region of size %lu, rounding down to %lu",
                    size, 1UL << masksize);
        }
        gdb.WriteWord(regs.DWT_COMP[comp], addr);
        gdb.WriteWord(regs.DWT_MASK[comp], masksize);
        gdb.WriteWord(regs.DWT_FUNCTION[comp], 0x3);
        LOG_INFO("Watching %s at %#x, size %lu (%lu bit mask)",
                expression.c_str(), addr, size, masksize);
        comp++;
    }

    gdb.Run();

    openthread.join();
    LOG_DEBUG("Reading from pipe");
    while (!TimeToExit) {
        timeval to = { 0, 100000 };
        fd_set readfd;
        FD_SET(PipeFd, &readfd);
        select(PipeFd + 1, &readfd, NULL, NULL, &to);

        char buf[128];
        ssize_t readres = read(PipeFd, buf, sizeof(buf));
        if (readres > 0) {
            // LOG_DEBUG("Read %ld bytes", readres);
            tfp.Feed(reinterpret_cast<const uint8_t*>(buf), readres);
        }
        else if (readres == -1) {
            if (errno != EAGAIN) {
                LOG_ERROR("Error when reading: %s", strerror(errno));
                break;
            }
        }
    }

    LOG_INFO("Exiting");

    gdb.Stop();
    sleep(1);
    gdb.DisableTpiu();

    return 0;
}

void CortexWatch::Exit()
{
    TimeToExit = true;
}

// -----------------------------------------------------------------

Pipe::Pipe() throw(std::runtime_error) :
        Fd(-1)
{
    Name = "/tmp/tpiu.log";
    int fifores = mkfifo(Name.c_str(), 0644);

    if (fifores != 0) {
        LOG_ERROR("Failed to create FIFO: %s", strerror(errno));
        throw std::runtime_error(strerror(errno));
    }
}

int Pipe::OpenForReading()
{
    assert(Fd == -1);
    Fd = open(Name.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (Fd == -1) {
        LOG_ERROR("Failed to open pipe: %s", strerror(errno));
    }
    return Fd;
}

Pipe::~Pipe()
{
    if (Fd == -1) {
        return;
    }

    close(Fd);

    LOG_DEBUG("Removing FIFO file");
    int unlinkres = unlink(Name.c_str());

    if (unlinkres != 0) {
        LOG_WARNING("Failed to remove FIFO file");
    }
}

// -----------------------------------------------------------------

static void printHelp(const char* progname)
{
    printf("Usage: %s [-h] -e PATH [-g PATH] [-w EXPRESSION [-w...]]\n"
            "  -h            Print this help text\n"
            "  -e PATH       Path to the ELF file to debug\n"
            "  -g PATH       Path to the GDB executable to use (%s)\n"
            "  -t STRING     GDB target specifier (%s)\n"
            "  -f HZ         CPU core frequency (%lu)\n"
            "  -w EXPRESSION C expression to watch, such as a variable or address\n"
            "       Variables can be specified by name, while memory addresses\n"
            "       should be given a type to indicate the size:\n"
            "              \'*(uint64_t*)0x20000000\'\n"
            "       It is prudent to enclose the expression in single quotes\n"
            "       to prevent the shell from performing path expansion on it.\n"
            "\n",
            progname, DEFAULT_GDB, DEFAULT_GDB_TARGET, DEFAULT_CORE_FREQ);
}

static void termhandler(int)
{
    LOG_DEBUG("SIGTERM");
    s_cortexWatch.Exit();
}

int main(int argc, char* argv[])
{
    std::string gdbPath = DEFAULT_GDB;
    std::string gdbTarget = DEFAULT_GDB_TARGET;
    std::string elfPath;
    size_t corefreq = DEFAULT_CORE_FREQ;
    std::vector<std::string> watch;

    int c;
    while ((c = getopt(argc, argv, "hg:t:e:f:w:")) != -1) {
        switch (c) {
        case 'g':
            gdbPath = optarg;
            break;
        case 't':
            gdbTarget = optarg;
            break;
        case 'e':
            elfPath = optarg;
            break;
        case 'f':
            corefreq = std::stoul(optarg);
            break;
        case 'w':
            watch.push_back(optarg);
            break;
        case 'h':
        default:
            printHelp(argv[0]);
            exit(1);
        }
    }

    if (elfPath.empty()) {
        printHelp(argv[0]);
        return 1;
    }

    struct sigaction act;
    act.sa_handler = termhandler;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    return s_cortexWatch.Run(gdbPath, gdbTarget, elfPath, corefreq, watch);
}
