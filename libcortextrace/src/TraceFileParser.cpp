#include <cassert>
#include <cstring>

#include "TraceEvent.h"
#include "TraceEventListener.h"
#include "TraceFileParser.h"

#include "log.h"

namespace lct {


TraceFileParser::TraceFileParser(TraceEventListener& listener) :
        Listener(listener),
        CurrentData(NULL), CurrentLen(0), CurrentPtr(0),
        OldLen(0), OldPtr(0)
{
}

TraceFileParser::~TraceFileParser()
{
}

void TraceFileParser::Feed(const uint8_t* data, size_t len)
{
    // LOG_DEBUG("Eating %lu bytes + %lu saved", len, OldLen);
    assert(OldPtr == 0);

    CurrentData = data;
    CurrentLen = len;
    CurrentPtr = 0;

    while (Parse())
        ;

    if (OldPtr < OldLen) {
        const size_t bytes = OldLen - OldPtr;
        LOG_DEBUG("Moving down %lu old bytes", bytes);
        memmove(OldData, &OldData[OldPtr], bytes);
        OldLen = bytes;
    }
    else {
        OldLen = 0;
    }
    OldPtr = 0;

    if (CurrentPtr < CurrentLen) {
        const size_t bytes = CurrentLen - CurrentPtr;
        // LOG_DEBUG("Keeping %lu bytes", bytes);
        memmove(&OldData[OldPtr], &CurrentData[CurrentPtr], bytes);
        CurrentPtr += bytes;
        OldLen += bytes;
    }

    assert(CurrentPtr == CurrentLen);
}

bool TraceFileParser::GetData(uint8_t* out, size_t count)
{
    const size_t have = (OldLen - OldPtr) + (CurrentLen - CurrentPtr);

    // LOG_DEBUG("Reading %lu bytes of %lu", count, have);

    if (count > have) {
        return false;
    }

    while (OldPtr < OldLen) {
        *out++ = OldData[OldPtr++];
        count--;
        if (!count) {
            return true;
        }
    }

    memcpy(out, &CurrentData[CurrentPtr], count);
    CurrentPtr += count;

    assert(CurrentPtr <= CurrentLen);
    return true;
}

void TraceFileParser::PutBackData(size_t count)
{
    // LOG_DEBUG("Putting back %lu bytes", count);

    if (CurrentPtr) {
        const size_t putBack = std::min(CurrentPtr, count);
        CurrentPtr -= putBack;
        count -= putBack;
    }
    if (count && OldPtr) {
        const size_t putBack = std::min(OldPtr, count);
        OldPtr -= putBack;
        count -= putBack;
    }
    assert(count == 0);
}

bool TraceFileParser::Parse()
{
    uint8_t b;
    bool ok = GetData(&b, 1);

    if (!ok) {
        return false;
    }

    if (b == 0x00) {
        ok = ParseSync();
    }
    else if (b == 0x70) {
        // LOG_DEBUG("Overflow");
        TraceEvent e(TraceEvent::TRACE_EVENT_OVERFLOW);
        Listener.HandleTraceEvent(e);
    }
    else {
        switch (b & 0x0f) {
        case 0x00: // Time Stamp
            ok = ParseTimeStamp(b);
            break;
        case 0x08: // ITM Extension
        case 0x0c: // DWT Extension
        case 0x04: // Reserved
            ok = ParseUnhandledExtension(b);
            break;
        default:
            if (b & 0x04) { // Hardware source
                ok = ParseHardwareSource(b);
            }
            else { // Instrumentation
                ok = ParseInstrumentation(b);
            }
            break;
        }
    }

    if (!ok) {
        PutBackData(1);
        return false;
    }

    return true;
}

bool TraceFileParser::ParseSync()
{
    uint8_t data[6] = { 0x00 };
    bool ok = GetData(&data[1], 5);

    if (!ok) {
        return false;
    }

    // LOG_DEBUG("SYNC?");
    for (int i = 0; i < 5; i++) {
        if (data[i] != 0x00) {
            // LOG_DEBUG("Broken SYNC");
            PutBackData(5);
            return true;
        }
    }
    if (data[5] != 0x00) {
        // LOG_DEBUG("Broken SYNC 2");
        PutBackData(5);
        return true;
    }

    TraceEvent e(TraceEvent::TRACE_EVENT_SYNC);
    Listener.HandleTraceEvent(e);

    return true;
}

bool TraceFileParser::ParseTimeStamp(uint8_t b)
{
    if ((b & 0x80) == 0) {
        // Timestamp packet format 2
        uint32_t value = (b >> 4) & 0x07;

        // LOG_DEBUG("Timestamp short: %#x", value);
        TraceEvent e(TraceEvent::TRACE_EVENT_TIMESTAMP, 0, value);
        Listener.HandleTraceEvent(e);

        return true;
    }

    uint8_t data[5] = { b };
    size_t i;
    for (i = 1; i < 5; i++) {
        bool ok = GetData(&data[i], 1);

        if (!ok) {
            PutBackData(i - 1);
            return false;
        }

        if (!(data[i] & 0x80)) {
            // Data does not continue
            break;
        }
    }

    const uint8_t code = (b >> 4) & 0x03;

    uint32_t value = ((data[4] & 0x7f) << 21) |
            ((data[3] & 0x7f) << 14) |
            ((data[2] & 0x7f) << 7) |
            ((data[1] & 0x7f));

    // LOG_DEBUG("Timestamp %luB code %#x: %#x", i, code, value);
    TraceEvent e(TraceEvent::TRACE_EVENT_TIMESTAMP, code, value);
    Listener.HandleTraceEvent(e);

    return true;
}

bool TraceFileParser::ParseInstrumentation(uint8_t b)
{
    const size_t len = 1 << ((b & 0x03) - 1);
    uint8_t data[4] = {0};
    bool ok = GetData(data, len);

    if (!ok) {
        return false;
    }

    const uint32_t intvalue = (data[3] << 24) |
            (data[2] << 16) |
            (data[1] << 8) |
            (data[0]);

    // LOG_DEBUG("Stim len %lu on port %u: %#x", len, b >> 3, intvalue);
    TraceEvent e(TraceEvent::TRACE_EVENT_INSTR, b, intvalue);
    Listener.HandleTraceEvent(e);

    return true;
}

bool TraceFileParser::ParseHardwareSource(uint8_t b)
{
    const size_t len = 1 << ((b & 0x03) - 1);
    uint8_t data[4] = {0};
    bool ok = GetData(data, len);

    if (!ok) {
        return false;
    }

    const uint32_t intvalue = (data[3] << 24) |
            (data[2] << 16) |
            (data[1] << 8) |
            (data[0]);

    TraceEvent e(TraceEvent::TRACE_EVENT_HW, b, intvalue);
    Listener.HandleTraceEvent(e);

    return true;
}

bool TraceFileParser::ParseUnhandledExtension(uint8_t b)
{
    uint8_t data[5] = { b };

    if (!(data[0] & 0x80)) {
        // Data does not continue
        return true;
    }

    size_t i;
    for (i = 1; i < 5; i++) {
        bool ok = GetData(&data[i], 1);

        if (!ok) {
            PutBackData(i - 1);
            return false;
        }

        if (!(data[i] & 0x80)) {
            // Data does not continue
            return true;
        }
    }

    return true;
}

} /* namespace lct */
