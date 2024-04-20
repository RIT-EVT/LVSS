#ifndef LVSS_ACS71240_HPP
#define LVSS_ACS71240_HPP

#include <EVT/io/ADC.hpp>
#include <EVT/utils/log.hpp>

namespace IO = EVT::core::IO;
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
    ACS71240(IO::ADC& adc0);

    /**
      * Get the current detected by the ACS71240
      *
      * @return The current
      */
    int32_t readCurrent();
    uint32_t readCounts();

private:
    /** ADC instance for getting input voltage */
    IO::ADC& ADC;
};

}// namespace LVSS

#endif
