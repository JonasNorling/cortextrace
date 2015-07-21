#include <unistd.h>
#include <cassert>

#include "GdbConnection.h"

#include "GdbConnectionState.h"
#include "log.h"

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

	// FIXME: We may need to send -gdb-set mi-async 1

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

void GdbConnection::EnableTpiu()
{
	{
		const char* argv[] = { "monitor", "tpiu", "config", "internal",
				"/tmp/tpiu.log", "uart", "off", "72000000" };
		GdbCommand cmd(CLIBypass(8, const_cast<char**>(argv)));
		State->SyncCommand(cmd);
	}

	{
		const char* argv[] = { "monitor", "itm", "ports", "on"};
		GdbCommand cmd(CLIBypass(4, const_cast<char**>(argv)));
		State->SyncCommand(cmd);
	}

	LOG_INFO("CPU ID: %#x", ReadWord(0xe000ed00));
	LOG_INFO("DEMCR: 0x%08x", ReadWord(0xe000edfc));
	LOG_INFO("ITM_TER0: 0x%08x", ReadWord(0xe0000e00));
	LOG_INFO("ITM_TCR: 0x%08x", ReadWord(0xe0000e80));
	LOG_INFO("TPIU_SSPSR: 0x%08x", ReadWord(0xe0040000));
	LOG_INFO("TPIU_ACPR: 0x%08x", ReadWord(0xe0040010));
	LOG_INFO("TPIU_SPPR: 0x%08x", ReadWord(0xe00400f0));
	LOG_INFO("TPIU_TYPE: 0x%08x", ReadWord(0xe0040fc8));
}

void GdbConnection::Run()
{
	LOG_DEBUG("Run");
	GdbCommand cmd(MIExecContinue());
	State->SyncCommand(cmd);
}

uint32_t GdbConnection::ReadWord(uint32_t address)
{
	char adrstring[16];
	sprintf(adrstring, "%#x", address);

	GdbCommand cmd(MIDataReadMemory(0, adrstring, const_cast<char*>("u"), 4, 1, 1, NULL));
	bool cmdres = State->SyncCommand(cmd);
	if (!cmdres) {
		return 0;
	}

	MIResultRecord* res = MICommandResult(cmd);
	MIString* mistr = MIResultRecordToString(res);
	// LOG_DEBUG("Read word: %s", MIStringToCString(mistr));
	MIStringFree(mistr);

	uint32_t value = 0;
	MIResult* r;
	for (MIListSet(res->results); (r = static_cast<MIResult*>(MIListGet(res->results)));) {
		if (std::string("memory") != r->variable) {
			continue;
		}
		if (r->value->type != MIValueTypeList) {
			LOG_WARNING("Weird memory dump");
			continue;
		}

		MIValue* row;
		for (MIListSet(r->value->values); (row = static_cast<MIValue*>(MIListGet(r->value->values)));) {
			if (row->type != MIValueTypeTuple) {
				continue;
			}

			assert(row->results);
			MIResult* item;
			for (MIListSet(row->results); (item = static_cast<MIResult*>(MIListGet(row->results)));) {
				if (std::string("data") != item->variable || item->value->type != MIValueTypeList) {
					continue;
				}

				MIValue* dataItem;
				for (MIListSet(item->value->values); (dataItem = static_cast<MIValue*>(MIListGet(item->value->values)));) {
					assert(dataItem->type == MIValueTypeConst);
					value = atoi(dataItem->cstring);
					// LOG_DEBUG("Data value: %s", dataItem->cstring);
				}
			}
		}
	}

	return value;
}

} // namespace
