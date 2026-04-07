#include <dev/ACS71240.hpp>

namespace LVSS {

ACS71240::ACS71240(io::ADC& adc0) : ADC(adc0)   {}

uint16_t ACS71240::readCurrent() {
    // Gets adcCounts from adc
    int16_t adcCounts = ADC.readRaw();

    //(((adcCounts - average adc counts) * inputVoltage) / (sensitivity * 2^12)/100)
    int16_t current = (((adcCounts - avgAdcCount) * voltIn) / (sensitivity << 12)/100);

    return current;
}

} // namespace LVSS