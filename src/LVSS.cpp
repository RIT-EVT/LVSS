#include <LVSS/LVSS.hpp>

namespace LVSS {

LVSS::LVSS(IO::GPIO& lvssEn, IO::ADC& adc0) : LVSS_EN(lvssEn), ADC(adc0) {
    //TODO: Implement
}

uint8_t LVSS::getNodeID() {
    return NODE_ID;
}

uint8_t LVSS::getNumElements() {
    return 0;
}

CO_OBJ_T* LVSS::getObjectDictionary() {
    return nullptr;
}

void LVSS::process() {
}

uint32_t LVSS::readCurrent(IO::ADC& adc0) {
    uint32_t current, voltage = static_cast<uint32_t>(adc0.read() * 1000);
    uint32_t r = (50 * 10 ^ -3), r2 = 5000, r3 = 50000;

    //Current = (Vout * 5k ohms) / (Rshunt * R3)
    current = ((voltage * r2) / (r * r3));

    return current;
}

}// namespace LVSS