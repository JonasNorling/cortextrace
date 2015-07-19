#pragma once
#include <cstdint>

namespace lct {

/**
 * Events parsed with TraceFileParser.
 */
class TraceEvent {
public:
	enum Type {
		/// Instrumentation event (write to ITM_STIM registers)
		TRACE_EVENT_INSTR,
		/// DWT event (exception trace, PC sampling, data trace)
		TRACE_EVENT_HW,
		/// Timestamp
		TRACE_EVENT_TIMESTAMP,
		/// Overflow - event buffers were full
		TRACE_EVENT_OVERFLOW,
		/// Sync packet
		TRACE_EVENT_SYNC,
	};

	TraceEvent(Type type, uint32_t code = 0, uint32_t value = 0) :
		Type(type), Value(value), Code(code)
	{ }
	virtual ~TraceEvent();

	const Type Type;
	const uint32_t Value;
	const uint32_t Code;
};

} /* namespace lct */
