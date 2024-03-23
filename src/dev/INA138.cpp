#include <dev/INA138.hpp>

namespace LVSS {

INA138::INA138(IO::ADC& adc0) : ADC(adc0) {}

uint32_t INA138::readCurrent() {
    //Gets adcCounts from adc
    uint32_t adcCounts = static_cast<uint32_t>(ADC.read() * 1000);

    // Current = ((adcCounts * 3.3) * 5k ohms * 100) / ((Rshunt * 100) * R3 * 4096)
    uint32_t current = ((adcCounts * r1 * 330) / (rShunt * r3 * 4096));

    return current;
}

}// namespace LVSS