#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#define class ClassRedefined
#include "libmi/MI.h"
#undef class
}


namespace lct {

class GdbResult {
public:
    GdbResult(GdbResult&& o) = default;
    static GdbResult FromResultRecord(MIResultRecord* rr);
    static GdbResult FromList(MIList* list);
    static GdbResult FromTuple(MIList* tuple);

    const GdbResult& operator[](std::string) const throw(std::out_of_range);
    const GdbResult& operator[](size_t) const throw(std::out_of_range);
    std::string Str() const throw(std::out_of_range);

private:
    GdbResult(std::string);
    GdbResult();

    std::unique_ptr<std::vector<GdbResult>> List;
    std::unique_ptr<std::unordered_multimap<std::string, GdbResult>> Tuple;
    std::unique_ptr<std::string> String;
};

class GdbCommand {
public:
    GdbCommand(MICommand* cmd);
    virtual ~GdbCommand();
    operator MICommand*();
    GdbResult Result();

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
