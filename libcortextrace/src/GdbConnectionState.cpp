#include <unistd.h>

#include "GdbConnectionState.h"
#include "log.h"

namespace lct {

GdbConnectionState::GdbConnectionState() :
		Gdb(NULL)
{
}

GdbConnectionState::~GdbConnectionState()
{
	MISessionFree(Gdb);
}

bool GdbConnectionState::SyncCommand(GdbCommand& cmd)
{
	// LOG_DEBUG("%s", MICommandToString(cmd));

	MISessionSendCommand(Gdb, cmd);
	do {
		MISessionProgress(Gdb);
	} while (!MICommandCompleted(cmd));

	if (!MICommandResultOK(cmd)) {
		LOG_WARNING("Command failed: %s: %s", MICommandToString(cmd),
				MICommandResultErrorMessage(cmd));
		return false;
	}

	return true;
}

GdbCommand::GdbCommand(MICommand* cmd) :
		Cmd(cmd)
{
}

GdbCommand::~GdbCommand()
{
	MICommandFree(Cmd);
}

GdbCommand::operator MICommand*()
{
	return Cmd;
}

} // namespace
