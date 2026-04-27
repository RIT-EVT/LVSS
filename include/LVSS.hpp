#ifndef _LVSS_
#define _LVSS_

#include <LVSS.hpp>

#include <cstring>

#include <core/dev/LCD.hpp>
#include <core/io/CANOpenMacros.hpp>
#include <core/io/CANopen.hpp>
#include <core/utils/log.hpp>

#include <dev/ACS71240.hpp>
#include <dev/TPS2HB35BQ.hpp>

namespace io   = core::io;
namespace dev  = core::dev;
namespace time = core::time;

namespace LVSS {

static constexpr uint8_t POWER_SWITCHES_SIZE = 3;
/**
 * This is the main board class for the LVSS.
 * Updates the power switch temperature, current, and fault values.
 */
class LVSS : public CANDevice {
public:
    /** Union bit field to hold a bit representing which boards are on/off */
    typedef union {
        uint16_t val;
        struct {
            // Power Switch 0
            uint8_t batt : 1;
            uint8_t hib  : 1;

            // Power Switch 1
            uint8_t tms  : 1;
            uint8_t hudl : 1;

            // Power Switch 2
            uint8_t gub : 1;
            uint8_t acc : 1;
        } __attribute__((packed));
    } BoardPowerState_t;

    /** Struct to hold the data for individual boards */
    typedef struct {
        uint16_t battCurrent;
        uint16_t hibCurrent;
        uint16_t tmsCurrent;
        uint16_t hudlCurrent;
        uint16_t accCurrent;
        uint16_t gubCurrent;

        int16_t switch0Temp;
        int16_t switch1Temp;
        int16_t switch2Temp;
    } switchData_t;

    typedef union {
        uint16_t val;
        struct {
            // Power Switch 0
            uint16_t battCurrentFault : 1;
            uint16_t hibCurrentFault  : 1;

            // Power Switch 1
            uint16_t tmsCurrentFault  : 1;
            uint16_t hudlCurrentFault : 1;

            // Power Switch 2
            uint16_t accCurrentFault : 1;
            uint16_t gubCurrentFault : 1;

            uint16_t switch0TempFault : 1;
            uint16_t switch1TempFault : 1;
            uint16_t switch2TempFault : 1;
        } __attribute__((packed));
    } switchFaults_t;

    /** FSM State declaration */
    enum class State {
        INITIALIZATION = 0u,
        RUNNING        = 1u,
    };

    static constexpr uint8_t NODE_ID         = 1;
    static constexpr uint8_t VCU_NODE_ID     = 0;
    static constexpr io::Pin VICOR_FAULT_PIN = io::Pin::PB_4;
    static constexpr io::Pin VICOR_SNS_PIN   = io::Pin::PA_4;

    /** Vicor fault pin */
    io::GPIO& vicorFT;
    io::GPIO::State VICOR_FAULT_ACTIVE_STATE = io::GPIO::State::HIGH;

    /**
     * Constructor for the LVSS class, takes a pointer to an array of power switches
     *
     * @param powerSwitches[in] an array of pointers to power switches
     * @param vicorFT[in] fault status pin of the vicor
     * @param acs71240[in] vicor current sensing IC
     */
    explicit LVSS(TPS2HB35BQ* powerSwitches[POWER_SWITCHES_SIZE], io::GPIO& vicorFT, ACS71240 acs71240);

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

    /** Holds the value of each board to enable/disable */
    BoardPowerState_t boardEN;

    /** Vicor Current */
    ACS71240 acs71240;

    /** Holds data for individual boards */
    switchData_t PowerSwitchState;

    /** Holds faults for individual boards */
    switchFaults_t PowerSwitchFaults;

    /** Tracks signal from VCU */
    uint16_t VCUBoardSig = 0;

    /** Tracks high value current */
    uint16_t battPackCurrent = 0x00;

    /** Tracks power switch fault */
    uint16_t switchFaultStatus = 0x00;

    /** The current state of the LVSS */
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
     * State: State::RUNNING
     */
    void runningState();

    /**
     * Have to know the size of the object dictionary for initialization
     * process.
     */
    static constexpr uint8_t OBJECT_DICTIONARY_SIZE       = 64;
    CO_OBJ_T objectDictionary[OBJECT_DICTIONARY_SIZE + 1] = {
        MANDATORY_IDENTIFICATION_ENTRIES_1000_1014,
        HEARTBEAT_PRODUCER_1017(2000),
        IDENTITY_OBJECT_1018,
        SDO_CONFIGURATION_1200,

        /**
         * Sets up the first RPDO to be an async trigger
         * TPDO 0 of the VCU_NODE_ID
         */
        RECEIVE_PDO_SETTINGS_OBJECT_140X(0, 0, VCU_NODE_ID, RECEIVE_PDO_TRIGGER_ASYNC),

        RECEIVE_PDO_MAPPING_START_KEY_16XX(0, 1),
        RECEIVE_PDO_MAPPING_ENTRY_16XX(0, 1, PDO_MAPPING_UNSIGNED16),

        TRANSMIT_PDO_SETTINGS_OBJECT_18XX(0x00, TRANSMIT_PDO_TRIGGER_TIMER, TRANSMIT_PDO_INHIBIT_TIME_DISABLE, 1000),
        TRANSMIT_PDO_SETTINGS_OBJECT_18XX(0x01, TRANSMIT_PDO_TRIGGER_TIMER, TRANSMIT_PDO_INHIBIT_TIME_DISABLE, 1000),
        TRANSMIT_PDO_SETTINGS_OBJECT_18XX(0x02, TRANSMIT_PDO_TRIGGER_TIMER, TRANSMIT_PDO_INHIBIT_TIME_DISABLE, 1000),

        ///////////////////////////////////////////////////////////////////////////
        // TPDO0: 4 × u16  (8 bytes) sending currents 0,1,2,3
        ///////////////////////////////////////////////////////////////////////////
        TRANSMIT_PDO_MAPPING_START_KEY_1AXX(0x00, 0x04),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x00, 0x01, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x00, 0x02, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x00, 0x03, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x00, 0x04, PDO_MAPPING_UNSIGNED16),

        ///////////////////////////////////////////////////////////////////////////
        // TPDO1: 3 × u16  (6 bytes) sending currents 4,5 and vicor current
        ///////////////////////////////////////////////////////////////////////////
        TRANSMIT_PDO_MAPPING_START_KEY_1AXX(0x01, 0x03),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x01, 0x01, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x01, 0x02, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x01, 0x03, PDO_MAPPING_UNSIGNED16),

        ///////////////////////////////////////////////////////////////////////////
        // TPDO2: 4 × u16  (8 bytes) sending temperatures 0,1,2 and board_en bitarray
        ///////////////////////////////////////////////////////////////////////////
        TRANSMIT_PDO_MAPPING_START_KEY_1AXX(0x02, 0x04),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x02, 0x01, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x02, 0x02, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x02, 0x03, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x02, 0x04, PDO_MAPPING_UNSIGNED16),

        ///////////////////////////////////////////////////////////////////////////
        // Data links: what those mapped entries point to in memory
        // (Group numbers 0x00..0x03 correspond to the TPDO number)
        ///////////////////////////////////////////////////////////////////////////

        // TPDO0 payload: HV Current Data (2×u16)
        DATA_LINK_START_KEY_21XX(LINK_TPDO_NUMBER(0x00), 0x04),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x00), 0x01, CO_TUNSIGNED16, &PowerSwitchState.battCurrent),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x00), 0x02, CO_TUNSIGNED16, &PowerSwitchState.hibCurrent),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x00), 0x03, CO_TUNSIGNED16, &PowerSwitchState.tmsCurrent),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x00), 0x04, CO_TUNSIGNED16, &PowerSwitchState.hudlCurrent),

        // TPDO1 payload: Power Switch rest of Currents & vicor
        DATA_LINK_START_KEY_21XX(LINK_TPDO_NUMBER(0x01), 0x04),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x01), 0x01, CO_TUNSIGNED16, &PowerSwitchState.accCurrent),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x01), 0x02, CO_TUNSIGNED16, &PowerSwitchState.gubCurrent),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x01), 0x03, CO_TUNSIGNED16, &battPackCurrent),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x01), 0x04, CO_TUNSIGNED16, &PowerSwitchFaults),

        // TPDO2 payload: Temperature Data & board_en signals
        DATA_LINK_START_KEY_21XX(LINK_TPDO_NUMBER(0x02), 0x04),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x02), 0x01, CO_TUNSIGNED16, &PowerSwitchState.switch0Temp),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x02), 0x02, CO_TUNSIGNED16, &PowerSwitchState.switch1Temp),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x02), 0x03, CO_TUNSIGNED16, &PowerSwitchState.switch2Temp),
        DATA_LINK_21XX(LINK_TPDO_NUMBER(0x02), 0x04, CO_TUNSIGNED16, &boardEN),

        DATA_LINK_START_KEY_21XX(LINK_RPDO_NUMBER(0), 1),
        DATA_LINK_21XX(LINK_RPDO_NUMBER(0), 1, CO_TUNSIGNED16, &VCUBoardSig),

        CO_OBJ_DICT_ENDMARK,
    };
};

} // namespace LVSS
#endif