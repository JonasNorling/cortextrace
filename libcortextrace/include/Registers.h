#pragma once

namespace lct {

class CortexM3Registers {
public:
    typedef uint32_t RegisterAddress;

    class RegisterArray {
    public:
        RegisterArray(RegisterAddress base, unsigned pitch) :
            Base(base), Pitch(pitch) {}
        RegisterAddress operator[](size_t n) const { return Base + n * Pitch; }
    protected:
        RegisterAddress Base;
        unsigned Pitch;
    };

    static const RegisterAddress ITM_TER0 =   0xe0000e00;
    static const RegisterAddress ITM_TCR =    0xe0000e80;
    static const RegisterAddress CPUID =      0xe000ed00;
    static const RegisterAddress DEMCR =      0xe000edfc;

    static const RegisterAddress DWT_CTRL =   0xe0001000;
    const RegisterArray DWT_COMP =           {0xe0001020, 16};
    const RegisterArray DWT_MASK =           {0xe0001024, 16};
    const RegisterArray DWT_FUNCTION =       {0xe0001028, 16};

    static const RegisterAddress TPIU_SSPSR = 0xe0040000;
    static const RegisterAddress TPIU_ACPR =  0xe0040010;
    static const RegisterAddress TPIU_SPPR =  0xe00400f0;
    static const RegisterAddress TPIU_TYPE =  0xe0040fc8;
};

class Registers : public CortexM3Registers {
};

}
