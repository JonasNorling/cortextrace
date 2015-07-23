#include <iostream>

#include "TraceEvent.h"
#include "TraceEventListener.h"
#include "TraceFileParser.h"
#include "GdbConnection.h"
#include "log.h"

class CortexWatch : public lct::TraceEventListener {
public:
	CortexWatch() { }
	virtual ~CortexWatch();
	int Run(std::istream& input);

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

int CortexWatch::Run(std::istream& input)
{
	lct::GdbConnection gdb;
	lct::TraceFileParser tfp(*this);

	gdb.Connect("/usr/bin/arm-none-eabi-gdb",
			"/home/jonas/workspace/knobbox/fw/build/knobbox.elf");
	gdb.EnableTpiu();
	uint32_t watch = gdb.ResolveAddress("&Controllers.ActiveKnob");
	gdb.WriteWord(0xe0001020, watch); // DWT_COMP[0]
	gdb.WriteWord(0xe0001024, 2); // DWT_MASK[0]
	gdb.WriteWord(0xe0001028, 0x3); // DWT_FUNCTION[0]
	LOG_INFO("Watching %#x", watch);
	gdb.Run();

	while (!input.eof()) {
		char buf[128];
		input.read(buf, sizeof(buf));
		const auto len = input.gcount();
		// LOG_DEBUG("Read %lu bytes", len);
		tfp.Feed(reinterpret_cast<const uint8_t*>(buf), len);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	CortexWatch t;
	return t.Run(std::cin);
}
