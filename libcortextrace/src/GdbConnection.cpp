#include <unistd.h>
#include <cassert>

#include "GdbConnection.h"

#include "GdbConnectionState.h"
#include "log.h"
#include "Registers.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

namespace lct {

GdbConnection::GdbConnection() :
        State()
{
    State.reset(new GdbConnectionState);
    State->Gdb = MISessionNew();
    //MISessionSetDebug(1);
}

GdbConnection::~GdbConnection()
{
}

static void consoleCb(char* str)
{
    LOG_DEBUG("CONSOLE> %s", str);
}

static void logCb(char* str)
{
    LOG_DEBUG("LOG> %s", str);
}

static void targetCb(char* str)
{
    LOG_DEBUG("TARGET> %s", str);
}

static void execCb(char* str, MIList *)
{
    LOG_DEBUG("EXEC> %s", str);
}

static void statusCb(char* str, MIList *)
{
    LOG_DEBUG("STATUS> %s", str);
}

static void notifyCb(char* str, MIList *)
{
    LOG_DEBUG("NOTIFY> %s", str);
}

static void eventCb(MIEvent *)
{
    LOG_DEBUG("EVENT");
}

void GdbConnection::Connect(std::string gdb, std::string exec)
{
    MISessionRegisterConsoleCallback(State->Gdb, consoleCb);
    MISessionRegisterLogCallback(State->Gdb, logCb);
    MISessionRegisterTargetCallback(State->Gdb, targetCb);
    MISessionRegisterExecCallback(State->Gdb, execCb);
    MISessionRegisterStatusCallback(State->Gdb, statusCb);
    MISessionRegisterNotifyCallback(State->Gdb, notifyCb);
    MISessionRegisterEventCallback(State->Gdb, eventCb);

    int res;

    MISessionSetGDBPath(State->Gdb, const_cast<char*>(gdb.c_str()));
    res = MISessionStartLocal(State->Gdb, const_cast<char*>(exec.c_str()));

    if (res != 0) {
        LOG_ERROR("Can't connect to GDB: %s", MIGetErrorStr());
        return;
    }

    LOG_DEBUG("Connected to GDB");

    {
        GdbCommand cmd(MICommandNew(const_cast<char*>("-gdb-set mi-async"), MIResultRecordDONE));
        MICommandAddOption(cmd, const_cast<char*>("1"), NULL);
        State->SyncCommand(cmd);
    }

    {
        GdbCommand cmd(MIGDBSet(const_cast<char*>("confirm"), const_cast<char*>("off")));
        State->SyncCommand(cmd);
    }

    {
        GdbCommand cmd(MICommandNew(const_cast<char*>("-target-select"), MIResultRecordCONNECTED));
        MICommandAddOption(cmd, const_cast<char*>("remote"), NULL);
        MICommandAddOption(cmd, const_cast<char*>(":3333"), NULL);
        State->SyncCommand(cmd);
    }
}

void GdbConnection::DisableTpiu()
{
    const char* argv[] = { "monitor", "tpiu", "config", "disable" };
    GdbCommand cmd(CLIBypass(ARRAY_SIZE(argv), const_cast<char**>(argv)));
    State->SyncCommand(cmd);
}

void GdbConnection::EnableTpiu(std::string logfile)
{
    {
        const char* argv[] = { "monitor", "tpiu", "config", "internal",
                logfile.c_str(), "uart", "off", "72000000" };
        GdbCommand cmd(CLIBypass(ARRAY_SIZE(argv), const_cast<char**>(argv)));
        State->SyncCommand(cmd);
    }

    {
        const char* argv[] = { "monitor", "itm", "ports", "on"};
        GdbCommand cmd(CLIBypass(ARRAY_SIZE(argv), const_cast<char**>(argv)));
        State->SyncCommand(cmd);
    }

    Registers regs;

    const uint32_t cpuid = ReadWord(regs.CPUID);
    const uint32_t partno = (cpuid >> 4) & 0xfff;
    LOG_INFO("CPUID: %#x: %s %s%u r%up%u",
            cpuid,
            (cpuid >> 24) == 0x41 ? "ARM" : "unknown",
            (partno & 0xc30) == 0xc20 ? "Cortex-M" : "unknown",
            partno & 0x0f,
            (cpuid >> 20) & 0xf,
            cpuid & 0xf);

    LOG_INFO("DEMCR: 0x%08x", ReadWord(regs.DEMCR));
    LOG_INFO("ITM_TER0: 0x%08x", ReadWord(regs.ITM_TER0));
    LOG_INFO("ITM_TCR: 0x%08x", ReadWord(regs.ITM_TCR));
    LOG_INFO("TPIU_SSPSR: 0x%08x", ReadWord(regs.TPIU_SSPSR));
    LOG_INFO("TPIU_ACPR: 0x%08x", ReadWord(regs.TPIU_ACPR));
    LOG_INFO("TPIU_SPPR: 0x%08x", ReadWord(regs.TPIU_SPPR));
    LOG_INFO("TPIU_TYPE: 0x%08x", ReadWord(regs.TPIU_TYPE));

    LOG_INFO("ROMDWT: 0x%08x", ReadWord(regs.ROMDWT));
    LOG_INFO("ROMFPB: 0x%08x", ReadWord(regs.ROMFPB));
    LOG_INFO("ROMITM: 0x%08x", ReadWord(regs.ROMITM));
    LOG_INFO("ROMTPIU: 0x%08x", ReadWord(regs.ROMTPIU));
    LOG_INFO("ROMETM: 0x%08x", ReadWord(regs.ROMETM));

    LOG_INFO("DWT_CTRL: 0x%08x", ReadWord(regs.DWT_CTRL));
    LOG_INFO("DWT_COMP[0]: 0x%08x", ReadWord(regs.DWT_COMP[0]));
    LOG_INFO("DWT_MASK[0]: 0x%08x", ReadWord(regs.DWT_MASK[0]));
    LOG_INFO("DWT_FUNCTION[0]: 0x%08x", ReadWord(regs.DWT_FUNCTION[0]));
}

void GdbConnection::Run()
{
    LOG_DEBUG("Run");
    GdbCommand cmd(MIExecContinue());
    State->SyncCommand(cmd);
}

void GdbConnection::Stop()
{
    LOG_DEBUG("Stop");
    GdbCommand cmd(MIExecInterrupt());
    State->SyncCommand(cmd);
    LOG_DEBUG("Stopped");
}

std::string GdbConnection::Evaluate(std::string expression)
{
    std::string output;
    GdbCommand cmd(MIDataEvaluateExpression(const_cast<char*>(expression.c_str())));
    bool cmdres = State->SyncCommand(cmd);
    if (cmdres) {
        GdbResult r(cmd.Result());
        try {
            output = r["value"].Str();
        }
        catch (std::out_of_range &e) {
            LOG_ERROR("%s", e.what());
        }
    }
    return output;
}

uint32_t GdbConnection::ResolveAddress(std::string expression)
{
    uint32_t addr = 0;
    ReadWord(expression, &addr);
    // LOG_DEBUG("%s resolves to %#x", expression.c_str(), addr);
    return addr;
}

uint32_t GdbConnection::ReadWord(uint32_t address)
{
    char adrstring[16];
    sprintf(adrstring, "%#x", address);
    return ReadWord(adrstring);
}

uint32_t GdbConnection::ReadWord(std::string expression, uint32_t* outAddress)
{
    GdbCommand cmd(MIDataReadMemory(0, const_cast<char*>(expression.c_str()),
            const_cast<char*>("u"), 4, 1, 1, NULL));
    bool cmdres = State->SyncCommand(cmd);
    if (!cmdres) {
        return 0;
    }

    // MIResultRecord* res = MICommandResult(cmd);
    // MIString* mistr = MIResultRecordToString(res);
    // LOG_DEBUG("Read word: %s", MIStringToCString(mistr));
    // MIStringFree(mistr);

    GdbResult r(cmd.Result());

    if (outAddress) {
        *outAddress = strtol(r["addr"].Str().c_str(), NULL, 0);
    }
    const uint32_t value = atoi(r["memory"][0]["data"][0].Str().c_str());
    return value;
}

bool GdbConnection::WriteWord(uint32_t address, uint32_t value)
{
    char adrstring[16];
    sprintf(adrstring, "%#x", address);
    char datastring[16];
    uint32_t levalue = ((value >> 24) & 0xff) |
            ((value >> 8) & 0xff00) |
            ((value << 8) & 0xff0000) |
            ((value << 24) & 0xff000000);
    sprintf(datastring, "%08x", levalue);

    GdbCommand cmd(MIDataWriteMemoryBytes(adrstring, datastring));
    return State->SyncCommand(cmd);
}

} // namespace
