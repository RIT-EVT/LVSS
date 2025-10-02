#ifndef LVSS_ACS71240_HPP
#define LVSS_ACS71240_HPP

#include <EVT/io/ADC.hpp>
#include <EVT/utils/log.hpp>

namespace IO  = EVT::core::IO;
namespace DEV = EVT::core::DEV;
namespace log = EVT::core::log;

namespace LVSS {

/**
 * Class for LVSS current sensor
 */
class ACS71240 {
public:
    /**
     * Constructor for current sensing class
     */
    explicit ACS71240(IO::ADC& adc0);

    /**
     * Get the current detected by the ACS71240
     *
     * @return The current in mA
     */
    int32_t readCurrent();

private:
    /** ADC instance for getting input voltage */
    int32_t voltIn = 3300; //millivolts
    int32_t sensitivity = 44; //millivolts / amp
    int32_t avgAdcCount = 1970; //Background Noise
    IO::ADC& ADC;
};

} // namespace LVSS

#endif
