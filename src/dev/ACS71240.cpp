#include <dev/ACS71240.hpp>

namespace LVSS {

ACS71240::ACS71240(IO::ADC& adc0) : ADC(adc0) {}

int32_t ACS71240::readCurrent() {
    //Gets adcCounts from adc
    int32_t adcCounts = ADC.readRaw();
    constexpr int32_t voltIn = 3300; //millivolts
    constexpr int32_t sensitivity = 44; //millivolts / amp
    constexpr int32_t avgAdcCount = 1970; //Is this number like the background noise?

    //(((adcCounts - average adc counts) * inputVoltage) / (sensitivity * 2^12)/100)
    int32_t current = (((adcCounts - avgAdcCount) * voltIn) / (sensitivity << 12)/100);

    return current;
}

}// namespace LVSS