#ifndef LVSS_INA138_HPP
#define LVSS_INA138_HPP

#include "EVT/dev/LCD.hpp"
#include "EVT/io/ADC.hpp"
#include "EVT/io/CANopen.hpp"
#include "EVT/io/GPIO.hpp"
#include "EVT/io/SPI.hpp"
#include "EVT/utils/log.hpp"
#include "LVSS.hpp"

namespace IO = EVT::core::IO;
namespace DEV = EVT::core::DEV;
namespace log = EVT::core::log;

namespace LVSS {

/**
 * class for LVSS current sensor feature
 */
class INA138 {
public:
    /**
      * Constructor for current sensing class
      */
    INA138(IO::ADC& adc0);

    /**
      * Calculates current using the input voltage rail into the Vicor
      *
      * @return the current
      */
    uint32_t readCurrent(IO::ADC&);

private:
    //ADC variable for getting input voltage
    IO::ADC& ADC;

    ///Read INA138 datasheet page 3
    //Resistor variables
    static constexpr uint32_t fixedPoint = 1000;         //Use fixpoint variable to multiply everything by 1k
    static constexpr uint32_t rShunt = 0.05 * fixedPoint;//0.5 ohm resistor
    static constexpr uint32_t r1 = 5000 * fixedPoint;    //5k ohm
    static constexpr uint32_t r3 = 50000 * fixedPoint;   //50k ohm
};

}// namespace LVSS

#endif//LVSS_INA138_HPP
