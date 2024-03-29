#ifndef LVSS_INA138_HPP
#define LVSS_INA138_HPP

#include <EVT/io/ADC.hpp>
#include <EVT/utils/log.hpp>

namespace IO = EVT::core::IO;
namespace DEV = EVT::core::DEV;
namespace log = EVT::core::log;

namespace LVSS {

/**
 * Class for LVSS current sensor
 */
class INA138 {
public:
    /**
     * Constructor for current sensing class
     */
    INA138(IO::ADC& adc0);

    /**
      * Get the current detected by the INA
      *
      * @return The current
      */
    uint32_t readCurrent();

private:
    /** ADC instance for getting input voltage */
    IO::ADC& ADC;

    /** Read INA138 datasheet page 3 */
    /** Resistor variables */
    static constexpr uint32_t rShunt = 0.05 * 100;//0.05 ohm resistor
    static constexpr uint32_t r1 = 5000;          //5k ohm
    static constexpr uint32_t r3 = 50000;         //50k ohm
};

}// namespace LVSS

#endif
