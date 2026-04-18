#ifndef LVSS_ACS71240_HPP
#define LVSS_ACS71240_HPP

#include <core/io/ADC.hpp>
#include <core/utils/log.hpp>

namespace io  = core::io;
namespace dev = core::dev;
namespace log = core::log;

namespace LVSS {

/**
 * This class is used to control ACS71240 temperature sensor.
 * The ACS71240 is a current sensing IC used to sense the current going across the LVSS and Vicor.
 */
class ACS71240 {
public:
    /**
     * Constructor for current sensing class
     */
    explicit ACS71240(io::ADC& adc0);

    /**
     * Get the current detected by the ACS71240
     *
     * @return The current in mA
     */
    uint16_t readCurrent();

private:
    /** ADC instance for getting input voltage */
    int16_t adcVoltage    = 3300; // ADC voltage in millivolts
    int16_t sensitivity   = 44;   // millivolts / amp
    int16_t scaledCurrent = 180;  // Sensitivity multiplied by ADC resolution (4096 * 0.044)
    int16_t avgAdcCount   = 1970; // Background Noise
    io::ADC& ADC;
};

} // namespace LVSS

#endif
