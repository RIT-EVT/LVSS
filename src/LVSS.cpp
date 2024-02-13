#include <LVSS/LVSS.hpp>

namespace LVSS {

LVSS::LVSS(IO::GPIO& lvssEn) : LVSS_EN(lvssEn) {
    // TODO: Implement
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





}