#include <dev/ACS71240.hpp>

namespace LVSS {

ACS71240::ACS71240(IO::ADC& adc0) : ADC(adc0) {}

int32_t ACS71240::readCurrent() {
    //Gets adcCounts from adc
    int32_t adcCounts = ADC.readRaw();

    //((adcCounts - average adc counts) * 3300 / 180)
    int32_t current = (((adcCounts - 1970) * 3300) / 180);

    return current;
}

}// namespace LVSS