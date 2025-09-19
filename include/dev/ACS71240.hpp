#ifndef LVSS_ACS71240_HPP
#define LVSS_ACS71240_HPP

#include <EVT/io/ADC.hpp>
#include <EVT/utils/log.hpp>

namespace IO = EVT::core::IO;
namespace DEV = EVT::core::DEV;
namespace log = EVT::core::log;

namespace LVSS {

/**
 * Class for LVSS current sensor, 70% sure it is ACS71240KEXBLT-030B3-S specifically
 * Datasheet in datasheets folder
 */
class ACS71240 {
public:
    /**
     * Constructor for current sensing class
     */
    ACS71240(IO::ADC& adc0);

    /**
      * Get the current detected by the ACS71240
      *
      * @return The current in mA
      */
    int32_t readCurrent();

private:
    /** ADC instance for getting input voltage */
    IO::ADC& ADC;
};

}// namespace LVSS

#endif
