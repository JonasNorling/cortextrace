#include "log.h"
#include "TraceEvent.h"
#include "TraceEventListener.h"
#include "TraceFileParser.h"

class Test : public lct::TraceEventListener {
public:
	Test() { }
	virtual ~Test();
	int Run();

	// interface TraceEventListener
	void HandleTraceEvent(const lct::TraceEvent& event);
};

Test::~Test()
{
}

void Test::HandleTraceEvent(const lct::TraceEvent& event)
{
	LOG_DEBUG("Got event");
}

int Test::Run()
{
	LOG_INFO("Running TraceFileParser test");

	// Test instrumentation packets, divided into several
	// bursts of data
	{
		lct::TraceFileParser tfp(*this);
		uint8_t buf1[] = { 0x70, 0x01, 0x41, 0x02, 0x42 };
		tfp.Feed(buf1, sizeof(buf1));

		uint8_t buf2[] = { 0x43 };
		tfp.Feed(buf2, sizeof(buf2));

		uint8_t buf3[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
		tfp.Feed(buf3, sizeof(buf3));

		uint8_t buf4[] = { 0x00, 0x00, 0x80 };
		tfp.Feed(buf4, sizeof(buf4));
	}

	// Test time stamps
	{
		lct::TraceFileParser tfp(*this);
		uint8_t buf1[] = {
				0xc0, 0x00,
				0xc0, 0x81, 0x07,
				0xd0, 0xff };
		tfp.Feed(buf1, sizeof(buf1));

		uint8_t buf2[] = { 0xff, 0xff, 0x00, 0x10 };
		tfp.Feed(buf2, sizeof(buf2));
	}

	return 0;
}

int main()
{
	Test t;
	return t.Run();
}
