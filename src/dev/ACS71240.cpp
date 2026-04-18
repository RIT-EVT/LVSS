#include <dev/ACS71240.hpp>

namespace LVSS {

ACS71240::ACS71240(io::ADC& adc0) : ADC(adc0) {}

uint16_t ACS71240::readCurrent() {
    // Gets adcCounts from adc
    int16_t adcCounts = ADC.readRaw();

    // (((adcCounts - average adc counts) * 3.3) / (4096 * 0.044)) * 1000
    int16_t rawCurrent = (((adcCounts - avgAdcCount) * adcVoltage) / scaledCurrent);

    return rawCurrent;
}

} // namespace LVSS