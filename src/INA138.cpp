#include "INA138.hpp"

namespace LVSS {

INA138::INA138(IO::ADC& adc0) : ADC(adc0) {
}

uint32_t INA138::readCurrent(IO::ADC& adc0) {
    //Gets voltage from adc
    uint32_t voltage = static_cast<uint32_t>(adc0.read() * 1000);
    voltage *= fixedPoint;

    //Current = (Vout * 5k ohms) / (Rshunt * R3)
    uint32_t current = ((voltage * r1) / (rShunt * r3));

    //divide the current by 1k fixed point get the actual number
    current /= fixedPoint;

    return current;
}

}// namespace LVSS