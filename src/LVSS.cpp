#include <LVSS.hpp>

namespace LVSS {

uint8_t LVSS::getNodeID() {
    return NODE_ID;
}

uint8_t LVSS::getNumElements() {
    return 0;
}

CO_OBJ_T* LVSS::getObjectDictionary() {
    return nullptr;
}
LVSS::LVSS(TPS2HB50BQ1* powerSwitches) : powerSwitches(powerSwitches) {
}

// temporarily commented out to make compiler happy
//LVSS::LVSS(TPS2HB50BQ1 powerSwitchArr[POWER_SWITCHES_SIZE]) : PowerSwitches(powerSwitchArr), currentSensor(currentSensor) {
//}
//LVSS::LVSS(TPS2HB50BQ1* powerSwitches) : PowerSwitches(powerSwitches) {}
//
}
