#include <iostream>

#include "TraceEvent.h"
#include "TraceEventListener.h"
#include "TraceFileParser.h"
#include "log.h"

class CortexTrace : public lct::TraceEventListener {
public:
    CortexTrace() { }
    virtual ~CortexTrace();
    int Run(std::istream& input);

    // interface TraceEventListener
    void HandleTraceEvent(const lct::TraceEvent& event);
};

CortexTrace::~CortexTrace()
{
}

void CortexTrace::HandleTraceEvent(const lct::TraceEvent& event)
{
    switch (event.Type) {
    case lct::TraceEvent::TRACE_EVENT_INSTR:
        std::cout << static_cast<char>(event.Value);
        break;
    case lct::TraceEvent::TRACE_EVENT_HW: {
        const uint8_t discriminator = event.Code >> 3;
        switch (discriminator) {
        case 2: // PC sample
            std::cout << "PC: " << std::hex << event.Value << std::dec << std::endl;
            break;
        }
        break;
    }
    case lct::TraceEvent::TRACE_EVENT_OVERFLOW:
        std::cout << "Overflow" << std::endl;
        break;
    default:
        std::cout << "Event " << event.Type << ", "
        << event.Code << ", " << event.Value << std::endl;
    }
}

int CortexTrace::Run(std::istream& input)
{
    lct::TraceFileParser tfp(*this);

    while (!input.eof()) {
        char buf[1024];
        input.read(buf, sizeof(buf));
        const auto len = input.gcount();
        // LOG_DEBUG("Read %lu bytes", len);
        tfp.Feed(reinterpret_cast<const uint8_t*>(buf), len);
    }

    return 0;
}

int main(int argc, char* argv[])
{
    CortexTrace t;
    return t.Run(std::cin);
}
