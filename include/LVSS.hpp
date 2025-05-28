#ifndef _LVSS_
#define _LVSS_

#include <EVT/dev/LCD.hpp>
#include <EVT/io/CANOpenMacros.hpp>
#include <EVT/io/CANopen.hpp>
#include <EVT/utils/log.hpp>
#include <LVSS.hpp>
#include <cstdio>
#include <cstring>
#include <dev/ACS71240.hpp>
#include <dev/TPS2HB35BQ.hpp>

// Clang was removed because it adds unnecessary tabs in front of the following macros.
// clang-format off
//TODO: REMOVE ONCE SDOs ARE IN EVT-CORE!!!!
//Temporary fix for CAN SDO server to process requests!!!!
#define SDO_CONFIGURATION_1200                  \
{                                               \
/* Communication Object SDO Server */           \
.Key  = CO_KEY(0x1200, 0x00, CO_OBJ_D___R_),    \
.Type = CO_TUNSIGNED32,                         \
.Data = (CO_DATA) 0x02,                         \
},                                              \
{                                               \
/* SDO Server Request COBID */                  \
.Key  = CO_KEY(0x1200, 0x01, CO_OBJ_DN__R_),    \
.Type = CO_TUNSIGNED32,                         \
.Data = (CO_DATA) CO_COBID_SDO_REQUEST(),       \
},                                              \
{ /* SDO Server Response COBID */               \
.Key  = CO_KEY(0x1200, 0x02, CO_OBJ_DN__R_),    \
.Type = CO_TUNSIGNED32,                         \
.Data = (CO_DATA) CO_COBID_SDO_RESPONSE(),      \
}
//TODO: REMOVE ONCE RPDO FIX IS IN EVT-CORE!!!!
//Temporary fix for RPDOs being mapped to the same data as TPDOs!!!!
#define RECEIVE_PDO_MAPPING_ENTRY_16XX(RPDO_NUMBER, SUB_INDEX, DATA_SIZE)     \
{                                                                             \
.Key  = CO_KEY(0x1600 + RPDO_NUMBER, SUB_INDEX, CO_OBJ_D___R_),               \
.Type = CO_TUNSIGNED32,                                                       \
.Data = (CO_DATA) CO_LINK(0x2200 + RPDO_NUMBER, 0x00 + SUB_INDEX, DATA_SIZE), \
}
// clang-format on

namespace IO = EVT::core::IO;
namespace DEV = EVT::core::DEV;
namespace time = EVT::core::time;

namespace LVSS {

static constexpr uint8_t POWER_SWITCHES_SIZE = 3;

/**
 * This is an example of a class for a board
 */
class LVSS : public CANDevice {
public:
    static constexpr uint8_t NODE_ID = 1;
    static constexpr uint8_t VCU_NODE_ID = 0;
    static constexpr uint8_t TPDO_NODE_ID = 1;

    IO::GPIO& vicorFT;
    static constexpr IO::Pin vicorFaultPin = IO::Pin::PB_4;
    static constexpr IO::Pin vicorSNS = IO::Pin::PA_4;
    IO::GPIO::State VICOR_FAULT_ACTIVE_STATE = IO::GPIO::State::HIGH;

    /** Union bit field to hold a bit representing which boards are on/off */
    typedef union {
        uint16_t val;
        struct {
            // Power Switch 0
            uint8_t batt : 1;
            uint8_t hib : 1;

            // Power Switch 1
            uint8_t tms : 1;
            uint8_t hudl : 1;

            // Power Switch 2
            uint8_t gub : 1;
            uint8_t acc : 1;
        };
    } BoardPowerState_u;

    typedef union switchState {
        uint16_t battCurrent;
        uint16_t hibCurrent;
        uint16_t tmsCurrent;
        uint16_t hudlCurrent;
        uint16_t accCurrent;
        uint16_t gubCurrent;

        int16_t switch0Temp;
        int16_t switch1Temp;
        int16_t switch2Temp;
    };

    /** FSM State declaration */
    enum class State {
        INITIALIZATION = 0u,
        IDLE = 1u,
    };

    /**
     * Constructor for the LVSS class, takes a pointer to an array of power switches
     *
     * @param powerSwitches an array of pointers to power switches
     * @param vicorFT fault status pin of the vicor
     * @param acs71240 vicor current sensing IC
     */
    explicit LVSS(TPS2HB35BQ* powerSwitches[POWER_SWITCHES_SIZE], IO::GPIO& vicorFT, ACS71240 acs71240);

    CO_OBJ_T* getObjectDictionary() override;

    uint8_t getNumElements() override;

    uint8_t getNodeID() override;

    /**
     * Handle running the core logic of the LVSS
     */
    void process();

private:
    /** A struct for each power switch (of which there are 3) */
    TPS2HB35BQ* powerSwitches[POWER_SWITCHES_SIZE]{};

    BoardPowerState_u boardEN;

    ACS71240 acs71240;

    switchState PowerSwitchState;

    /** Tracks signal from VCU */
    uint16_t VCUBoardSig = 0;

    /** Tracks high value current */
    uint16_t battPackCurrent = 0x00;

    /** Tracks power switch fault */
    uint16_t switchFaultstatus = 0x00;

    /**
     * The current state of the LVSS
     */
    State state;

    /**
     * Boolean flag which represents that a state has just changed
     *
     * Useful for determining when operations that only take place once per
     * state change should take place.
     */
    bool isNewState = false;

    /**
     * Handle VICOR Initialization state
     * Waits for 101ms while the VICOR initializes.
     *
     * State: State::INITIALIZATION
     */
    void initState();

    /**
     * Checks LVSS values
     *
     * State: State::IDLE
     */
    void runningState();

    /**
     * Have to know the size of the object dictionary for initialization
     * process.
     */
    static constexpr uint8_t OBJECT_DICTIONARY_SIZE = 70;
    /**
     * The object dictionary itself. Will be populated by this object during
     * construction.
     *
     * The plus one is for the special "end of dictionary" marker.
     */
    CO_OBJ_T objectDictionary[OBJECT_DICTIONARY_SIZE + 1] = {
        MANDATORY_IDENTIFICATION_ENTRIES_1000_1014,
        HEARTBEAT_PRODUCER_1017(2000),
        IDENTITY_OBJECT_1018,
        SDO_CONFIGURATION_1200,

        RECEIVE_PDO_SETTINGS_OBJECT_140X(0x00, 0x00, VCU_NODE_ID, RECEIVE_PDO_TRIGGER_ASYNC),

        RECEIVE_PDO_MAPPING_START_KEY_16XX(0x00, 0x01),
        {
            .Key = CO_KEY(0x1600 + 0x00, 0x01, CO_OBJ_D___R_),
            .Type = CO_TUNSIGNED32,
            .Data = (CO_DATA) CO_LINK(0x2200 + 0x00, 0x00 + 0x01, PDO_MAPPING_UNSIGNED16),
        },

        TRANSMIT_PDO_SETTINGS_OBJECT_18XX(0x00, TRANSMIT_PDO_TRIGGER_TIMER, TRANSMIT_PDO_INHIBIT_TIME_DISABLE, 100),
        TRANSMIT_PDO_SETTINGS_OBJECT_18XX(0x01, TRANSMIT_PDO_TRIGGER_TIMER, TRANSMIT_PDO_INHIBIT_TIME_DISABLE, 1000),
        TRANSMIT_PDO_SETTINGS_OBJECT_18XX(0x02, TRANSMIT_PDO_TRIGGER_TIMER, TRANSMIT_PDO_INHIBIT_TIME_DISABLE, 1000),
        TRANSMIT_PDO_SETTINGS_OBJECT_18XX(0x03, TRANSMIT_PDO_TRIGGER_TIMER, TRANSMIT_PDO_INHIBIT_TIME_DISABLE, 1000),

        // TPDO 0 Map
        TRANSMIT_PDO_MAPPING_START_KEY_1AXX(0x00, 0x02),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x00, 0x01, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x00, 0x02, PDO_MAPPING_UNSIGNED16),

        // TPDO 1 Map
        TRANSMIT_PDO_MAPPING_START_KEY_1AXX(0x01, 0x04),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x01, 0x01, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x01, 0x02, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x01, 0x03, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x01, 0x04, PDO_MAPPING_UNSIGNED16),

        // TPDO 2 Map
        TRANSMIT_PDO_MAPPING_START_KEY_1AXX(0x02, 0x02),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x02, 0x01, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x02, 0x02, PDO_MAPPING_UNSIGNED16),

        // TPDO 3 Map
        TRANSMIT_PDO_MAPPING_START_KEY_1AXX(0x03, 0x03),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x03, 0x01, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x03, 0x02, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x03, 0x03, PDO_MAPPING_UNSIGNED16),

        // User defined data, this will be where we put elements that can be
        // accessed via SDO and depending on configuration PDO

        /** Transfer data */
        DATA_LINK_START_KEY_21XX(0x00, 0x02),
        DATA_LINK_21XX(0x00, 0x01, CO_TUNSIGNED16, &battPackCurrent),
        DATA_LINK_21XX(0x00, 0x02, CO_TUNSIGNED16, &switchFaultstatus),

        DATA_LINK_START_KEY_21XX(0x01, 0x04),
        DATA_LINK_21XX(0x01, 0x01, CO_TUNSIGNED16, &PowerSwitchState.battCurrent),
        DATA_LINK_21XX(0x01, 0x02, CO_TUNSIGNED16, &PowerSwitchState.hibCurrent),
        DATA_LINK_21XX(0x01, 0x03, CO_TUNSIGNED16, &PowerSwitchState.tmsCurrent),
        DATA_LINK_21XX(0x01, 0x04, CO_TUNSIGNED16, &PowerSwitchState.hudlCurrent),

        DATA_LINK_START_KEY_21XX(0x02, 0x02),
        DATA_LINK_21XX(0x02, 0x01, CO_TUNSIGNED16, &PowerSwitchState.accCurrent),
        DATA_LINK_21XX(0x02, 0x02, CO_TUNSIGNED16, &PowerSwitchState.gubCurrent),

        DATA_LINK_START_KEY_21XX(0x03, 0x03),
        DATA_LINK_21XX(0x03, 0x01, CO_TSIGNED16, &PowerSwitchState.switch0Temp),
        DATA_LINK_21XX(0x03, 0x02, CO_TSIGNED16, &PowerSwitchState.switch1Temp),
        DATA_LINK_21XX(0x03, 0x03, CO_TSIGNED16, &PowerSwitchState.switch2Temp),

        /** Receive data */
        DATA_LINK_START_KEY_21XX(0x100, 0x01),
        DATA_LINK_21XX(0x100, 0x01, CO_TUNSIGNED16, &VCUBoardSig),

        // End of dictionary marker
        CO_OBJ_DICT_ENDMARK,
    };
};

}// namespace LVSS
#endif