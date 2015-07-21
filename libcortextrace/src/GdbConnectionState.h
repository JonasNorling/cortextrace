#pragma once

extern "C" {
#define class ClassRedefined
#include "libmi/MI.h"
#undef class
}

namespace lct {

class GdbCommand {
public:
	GdbCommand(MICommand* cmd);
	virtual ~GdbCommand();
	operator MICommand*();

protected:
	MICommand* Cmd;

private:
	GdbCommand(const GdbCommand&);
	GdbCommand& operator=(const GdbCommand&);
};

class GdbConnectionState {
public:
	GdbConnectionState();
	virtual ~GdbConnectionState();

	bool SyncCommand(GdbCommand& cmd);

	MISession* Gdb;

private:
	GdbConnectionState(const GdbConnectionState&);
	GdbConnectionState& operator=(const GdbConnectionState&);
};

} // namespace
