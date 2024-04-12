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

void LVSS::process() {
}

}// namespace LVSS
