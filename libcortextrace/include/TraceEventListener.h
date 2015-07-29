#pragma once

namespace lct {

class TraceEvent;

/**
 * Interface for receiving events from TraceFileParser.
 */
class TraceEventListener
{
public:
    virtual ~TraceEventListener();
    virtual void HandleTraceEvent(const TraceEvent& event) = 0;
};

} /* namespace lct */
