#include <dev/ACS71240.hpp>

namespace LVSS {

ACS71240::ACS71240(IO::ADC& adc0) : ADC(adc0) {}

int32_t ACS71240::readCurrent() {
    //Gets adcCounts from adc
    int32_t adcCounts = ADC.readRaw();

    int32_t current = (((adcCounts-1970) * 3300) / 180);

    return current;
}

uint32_t ACS71240::readCounts() {
    uint32_t adcCounts = ADC.readRaw();
    return adcCounts;
}

}// namespace LVSS