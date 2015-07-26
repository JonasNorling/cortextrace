#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "TraceEvent.h"
#include "TraceEventListener.h"
#include "TraceFileParser.h"
#include "GdbConnection.h"
#include "log.h"

#define DEFAULT_GDB "arm-none-eabi-gdb"

class CortexWatch : public lct::TraceEventListener {
public:
	CortexWatch() { }
	virtual ~CortexWatch();
	int Run(std::string gdbPath, std::string elfPath,
	        const std::vector<std::string>& watch);

	// interface TraceEventListener
	void HandleTraceEvent(const lct::TraceEvent& event);
};

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

int CortexWatch::Run(std::string gdbPath, std::string elfPath,
        const std::vector<std::string>& watch)
{
	lct::GdbConnection gdb;
	lct::TraceFileParser tfp(*this);
	std::string logpipe("/tmp/tpiu.log");
	int fifores = mkfifo(logpipe.c_str(), 0644);

	if (fifores != 0) {
	    LOG_ERROR("Failed to create FIFO: %s", strerror(errno));
	    return 1;
	}

	gdb.Connect(gdbPath, elfPath);
	gdb.EnableTpiu(logpipe);

	const uint32_t dwt_ctrl = gdb.ReadWord(0xe0001000);
	const size_t numcomp = dwt_ctrl >> 28;

	LOG_DEBUG("%lu comparators on this chip", numcomp);

	size_t comp = 0;
	for (auto expression : watch) {
	    if (comp >= numcomp) {
	        LOG_ERROR("Too many expressions, hardware only has %lu comparators",
	                numcomp);
	        return 1;
	    }
	    const size_t size = std::stoul(gdb.Evaluate(std::string("sizeof(") +
	            expression + ")"));

	    const uint32_t addr = gdb.ResolveAddress(std::string("&(") + expression + ")");
	    const uint32_t regOfs = comp++ << 4;
	    gdb.WriteWord(0xe0001020 + regOfs, addr); // DWT_COMP[comp]
	    // FIXME: Write right mask
	    gdb.WriteWord(0xe0001024 + regOfs, 2); // DWT_MASK[comp]
	    gdb.WriteWord(0xe0001028 + regOfs, 0x3); // DWT_FUNCTION[comp]
	    LOG_INFO("Watching %s at %#x, size %lu", expression.c_str(), addr, size);
	}

	gdb.Run();

	std::ifstream input;
	input.open(logpipe);
	while (!input.eof()) {
		char buf[128];
		input.read(buf, sizeof(buf));
		const auto len = input.gcount();
		// LOG_DEBUG("Read %lu bytes", len);
		tfp.Feed(reinterpret_cast<const uint8_t*>(buf), len);
	}

	return 0;
}

static void printHelp(const char* progname)
{
    printf("Usage: %s [-h] -e PATH [-g PATH] [-w EXPRESSION [-w...]]\n"
            "  -h            Print this help text\n"
            "  -e PATH       Path to the ELF file to debug\n"
            "  -g PATH       Path to the GDB executable to use (%s)\n"
            "  -w EXPRESSION C expression to watch, such as a variable or address\n"
            "       Variables can be specified by name, while memory addresses\n"
            "       should be given a type to indicate the size:\n"
            "              \'*(uint64_t*)0x20000000\'\n"
            "       It is prudent to enclose the expression in single quotes\n"
            "       to prevent the shell from performing path expansion on it.\n"
            "\n",
            progname, DEFAULT_GDB);
}

int main(int argc, char* argv[])
{
    std::string gdbPath = DEFAULT_GDB;
    std::string elfPath;
    std::vector<std::string> watch;

    int c;
    while ((c = getopt(argc, argv, "hg:e:w:")) != -1) {
        switch (c) {
        case 'g':
            gdbPath = optarg;
            break;
        case 'e':
            elfPath = optarg;
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

    CortexWatch t;
	return t.Run(gdbPath, elfPath, watch);
}
