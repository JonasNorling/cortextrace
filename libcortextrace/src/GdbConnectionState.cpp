#include <cassert>
#include <unistd.h>

#include "log.h"
#include "GdbConnectionState.h"

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

GdbResult GdbCommand::Result()
{
	GdbResult r(GdbResult::FromResultRecord(MICommandResult(Cmd)));
	return r;
}

GdbResult::GdbResult() :
		List(), Tuple(), String()
{
}

GdbResult::GdbResult(std::string s) :
		List(), Tuple(), String(new std::string(s))
{
}

const GdbResult& GdbResult::operator[](std::string s) const throw(std::out_of_range)
{
	if (!Tuple) {
		throw std::out_of_range("Not found");
	}
	auto v = Tuple->find(s);
	if (v == Tuple->end()) {
		throw std::out_of_range("Not found");
	}
	return v->second;
}

const GdbResult& GdbResult::operator[](size_t n) const throw(std::out_of_range)
{
	if (!List) {
		throw std::out_of_range("Not found");
	}
	return List->at(n);
}

std::string GdbResult::Str() const throw(std::out_of_range)
{
	if (!String) {
		throw std::out_of_range("Not found");
	}
	return *String;
}

GdbResult GdbResult::FromResultRecord(MIResultRecord* rr)
{
	assert(rr->results);
	return GdbResult::FromTuple(rr->results);
}

GdbResult GdbResult::FromList(MIList* list)
{
	GdbResult out;
	out.List.reset(new std::vector<GdbResult>);

	// LOG_DEBUG("%u elements", list->l_nel);

	MIValue* v;
	for (MIListSet(list); (v = static_cast<MIValue*>(MIListGet(list)));) {
		switch (v->type) {
		case MIValueTypeConst: {
			// LOG_DEBUG("String: %s", v->cstring);
			out.List->push_back(std::move(GdbResult(v->cstring)));
			break;
		}
		case MIValueTypeList: {
			// LOG_DEBUG("List");
			assert(v->values);
			out.List->push_back(std::move(GdbResult::FromList(v->values)));
			break;
		}
		case MIValueTypeTuple: {
			// LOG_DEBUG("Tuple");
			assert(v->results);
			out.List->push_back(std::move(GdbResult::FromTuple(v->results)));
			break;
		}
		}
	}

	return out;
}

GdbResult GdbResult::FromTuple(MIList* tuple)
{
	GdbResult out;
	out.Tuple.reset(new std::unordered_multimap<std::string, GdbResult>);

	// LOG_DEBUG("%u elements", tuple->l_nel);

	MIResult* r;
	for (MIListSet(tuple); (r = static_cast<MIResult*>(MIListGet(tuple)));) {
		switch (r->value->type) {
		case MIValueTypeConst: {
			// LOG_DEBUG("String: %s = %s", r->variable, r->value->cstring);
			assert(r->value->cstring);
			std::pair<std::string, GdbResult> pair(r->variable, GdbResult(r->value->cstring));
			out.Tuple->insert(std::move(pair));
			break;
		}
		case MIValueTypeList: {
			// LOG_DEBUG("List: %s = [...]", r->variable);
			assert(r->value->values);
			std::pair<std::string, GdbResult> pair(r->variable, GdbResult::FromList(r->value->values));
			out.Tuple->insert(std::move(pair));
			break;
		}
		case MIValueTypeTuple: {
			// LOG_DEBUG("Tuple: %s = {...}", r->variable);
			assert(r->value->results);
			std::pair<std::string, GdbResult> pair(r->variable, GdbResult::FromTuple(r->value->results));
			out.Tuple->insert(std::move(pair));
			break;
		}
		}
	}

	return out;
}

} // namespace
