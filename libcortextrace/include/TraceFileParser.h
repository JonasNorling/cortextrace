#pragma once

#include <istream>
#include <cstdint>

namespace lct {

class TraceEventListener;

/**
 * Parse ARM ITM packets into more manageable units by parsing the raw binary
 * data stream.
 *
 * Information on the format can be found in the ARMv7-M Architecture
 * Reference Manual.
 *
 * To use this class, feed in raw binary data with the Feed() method and use
 * the TraceEventListener interface to receive the extracted events.
 */
class TraceFileParser {
public:
	TraceFileParser(TraceEventListener& listener);
	virtual ~TraceFileParser();

	void Feed(const uint8_t* data, size_t len);

protected:
	TraceEventListener& Listener;

	const uint8_t* CurrentData;
	size_t CurrentLen;
	size_t CurrentPtr;

	uint8_t OldData[10];
	size_t OldLen;
	size_t OldPtr;

	bool GetData(uint8_t* out, size_t count);
	void PutBackData(size_t count);

	bool Parse();
	bool ParseSync();
	bool ParseTimeStamp(uint8_t b);
	bool ParseInstrumentation(uint8_t b);
	bool ParseHardwareSource(uint8_t b);
	bool ParseUnhandledExtension(uint8_t b);

private:
	TraceFileParser(const TraceFileParser&);
	TraceFileParser& operator=(const lct::TraceFileParser&);
};

} /* namespace lct */
