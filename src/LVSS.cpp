#include <LVSS.hpp>

namespace LVSS {

uint8_t LVSS::getNodeID() {
    return NODE_ID;
}

uint8_t LVSS::getNumElements() {
    return OBJECT_DICTIONARY_SIZE + 1;
}

CO_OBJ_T* LVSS::getObjectDictionary() {
    return &objectDictionary[0];
}

LVSS::LVSS(TPS2HB35BQ* powerSwitches[POWER_SWITCHES_SIZE], IO::GPIO& vicorFT) : vicorFT(vicorFT), boardEN({0}), state(State::INITIALIZATION) {
    for (int i = 0; i < POWER_SWITCHES_SIZE; i++) {
        this->powerSwitches[i] = powerSwitches[i];
    }
}

/* LVSS Vicor DCM4623 State Machine */
void LVSS::process() {
    /* Assigns the VCU signal to the union bit field */
    this->boardEN.val = VCUBoardSig;

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Battery: %d\r\nHIB: %d\r\nTMS: %d\r\nHUDL: %d\r\nACC: %d\r\nGUB: %d\r\n", boardEN.batt, boardEN.hib, boardEN.tms, boardEN.hudl, boardEN.acc, boardEN.gub);

    /* Turn on boards */
    powerSwitches[0]->setPowerSwitchStates(boardEN.batt, boardEN.hib);// Turn on Battery and HIB
    powerSwitches[1]->setPowerSwitchStates(boardEN.tms, boardEN.hudl);// Turn on TMS and HUDL
    powerSwitches[2]->setPowerSwitchStates(boardEN.acc, boardEN.gub); // Turn on Acc and GUB

    /* Current Checking */
    PowerSwitchState.battCurrent = powerSwitches[0]->getChannel1Current();
    PowerSwitchState.tmsCurrent = powerSwitches[1]->getChannel1Current();
    PowerSwitchState.accCurrent = powerSwitches[2]->getChannel1Current();

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Switch 0 Channel 1: %d\r\nSwitch 1 Channel 1: %d\r\nSwitch 2 Channel 1: %d\r\n", PowerSwitchState.battCurrent, PowerSwitchState.tmsCurrent, PowerSwitchState.accCurrent);

    time::wait(2);// Power switches require the ADC to wait a min of 165 micro seconds before sampling SNS pin again

    PowerSwitchState.hibCurrent = powerSwitches[0]->getChannel2Current();
    PowerSwitchState.hudlCurrent = powerSwitches[1]->getChannel2Current();
    PowerSwitchState.gubCurrent = powerSwitches[2]->getChannel2Current();

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Switch 0 Channel 2: %d\r\nSwitch 1 Channel 2: %d\r\nSwitch 2 Channel 2: %d\r\n", PowerSwitchState.hibCurrent, PowerSwitchState.hudlCurrent, PowerSwitchState.gubCurrent);

    PowerSwitchState.switch0Temp = powerSwitches[0]->getTempandFault();
    PowerSwitchState.switch1Temp = powerSwitches[1]->getTempandFault();
    PowerSwitchState.switch2Temp = powerSwitches[2]->getTempandFault();

    log::LOGGER.log(log::Logger::LogLevel::INFO, "Switch 0 Temperature: %d\r\nSwitch 1 Temperature: %d\r\nSwitch 2 Temperature: %d\r\n", PowerSwitchState.switch0Temp, PowerSwitchState.switch1Temp, PowerSwitchState.switch2Temp);
}

}// namespace LVSS