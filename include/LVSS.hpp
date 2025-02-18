#ifndef _LVSS_
#define _LVSS_

#include <EVT/dev/LCD.hpp>
#include <EVT/io/CANopen.hpp>
#include <EVT/utils/log.hpp>
#include <LVSS.hpp>
#include <cstdio>
#include <cstring>
#include <dev/ACS71240.hpp>
#include <dev/TPS2HB50BQ1.hpp>

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
    static constexpr uint8_t NODE_ID = 42;
    static constexpr uint8_t TPDO_NODE_ID = 1;

    IO::GPIO& vicorFT;
    static constexpr IO::Pin vicorFaultPin = IO::Pin::PB_4;
    static constexpr IO::GPIO::State VICOR_FAULT_ACTIVE_STATE = IO::GPIO::State::HIGH;

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
    } u_t;

    /** FSM State declaration */
    enum class State {
        /** When LVSS is powered on */
        INITIALIZATION = 0u,
        /** Reads VCU signal and starting data liike switch current */
        SOFT_START = 1u,
        /** Turns on the specified boards */
        POWER_UP = 2u,
        /** Checks values for errors */
        IDLE = 3u,
        /** Handles errors */
        FAULT = 4u,
    };

    enum class PowerSwitchStatus {
        Safe = 1u,
        OverCurrent = 2u,
        Temperature = 3u,
        Fault = 4u
    };

    PowerSwitchStatus err[3] = {PowerSwitchStatus::Safe, PowerSwitchStatus::Safe, PowerSwitchStatus::Safe};// Holds the error status for each power switch

    /**
     * Constructor for the LVSS class, takes a pointer to an array of power switches
     * @param powerSwitches an array of pointers to power switches
     */
    explicit LVSS(TPS2HB50BQ1* powerSwitches[POWER_SWITCHES_SIZE], IO::GPIO& vicorFT);

    CO_OBJ_T* getObjectDictionary() override;

    uint8_t getNumElements() override;

    uint8_t getNodeID() override;

    /**
     * Handle running the core logic of the LVSS
     */
    void process();

private:
    TPS2HB50BQ1* powerSwitches[POWER_SWITCHES_SIZE]{};// a struct for each power switch (of which there are 3)

    u_t boardEN;

    /** Tracks signal from VCU */
    uint16_t VCUBoardSig = 0x00;

    /** Tracks high value current */
    uint16_t highValCurrent = 0x00;

    /** Tracks power switch current */
    uint16_t switchCurrent = 0x00;

    /** Tracks power switch temperature */
    uint16_t switchTemperature = 0x00;

    /** Tracks power switch fault */
    uint16_t switchFaultstatus = 0x00;

    /**
     * The current state of the LVSS
     */
    State state;

    uint8_t CurrentLim = 9000;   // Current Limit
    uint8_t TemperatureLim = 135;// Temperature Limit

    /**
     * Boolean flag which represents that a state has just changed
     *
     * Useful for determining when operations that only take place once per
     * state change should take place.
     */
    bool isNewState = false;

    /**
     * Handle VICOR Initialization state
     * Waits for 110ms while the VICOR initializes.
     *
     * State: State::INITIALIZATION
     */
    void initState();

    /**
     * Handle VICOR soft start state
     * Reads in CANopen messages from the VCU of which boards to turn on/ff.
     *
     * State: State::SOFT_START
     */
    void softStartState();

    /**
     * Handle VICOR when the LVSS sends out power to the boards
     * Determines which boards get turned on/off first based on priority.
     *
     * State: State::POWER_UP
     */
    void powerUpState();

    /**
     * Checks LVSS values
     *
     * State: State::IDLE
     */
    void idleState();

    /**
     * Checks LVSS values
     *
     * State: State::FAULT
     */
    void faultState();

    /**
     * Have to know the size of the object dictionary for initialization
     * process.
     */
    static constexpr uint8_t OBJECT_DICTIONARY_SIZE = 40;

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

        RECEIVE_PDO_SETTINGS_OBJECT_140X(0x00, 0x00, TPDO_NODE_ID, RECEIVE_PDO_TRIGGER_ASYNC),
        RECEIVE_PDO_MAPPING_START_KEY_16XX(0x00, 0x1),
        RECEIVE_PDO_MAPPING_ENTRY_16XX(0x00, 0x01, PDO_MAPPING_UNSIGNED16),

        TRANSMIT_PDO_SETTINGS_OBJECT_18XX(0x00, TRANSMIT_PDO_TRIGGER_TIMER, TRANSMIT_PDO_INHIBIT_TIME_DISABLE, 2000),
        TRANSMIT_PDO_MAPPING_START_KEY_1AXX(0x00, 0x02),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x00, 0x01, PDO_MAPPING_UNSIGNED16),
        TRANSMIT_PDO_MAPPING_ENTRY_1AXX(0x00, 0x02, PDO_MAPPING_UNSIGNED16),

        // User defined data, this will be where we put elements that can be
        // accessed via SDO and depending on configuration PDO
        DATA_LINK_START_KEY_21XX(0x00, 0x05),
        /** Receive data */
        DATA_LINK_21XX(0x00, 0x01, CO_TUNSIGNED16, &VCUBoardSig),

        /** Transfer data */
        DATA_LINK_21XX(0x00, 0x02, CO_TUNSIGNED16, &highValCurrent),
        DATA_LINK_21XX(0x00, 0x03, CO_TUNSIGNED16, &switchCurrent),
        DATA_LINK_21XX(0x00, 0x04, CO_TUNSIGNED16, &switchTemperature),
        DATA_LINK_21XX(0x00, 0x05, CO_TUNSIGNED16, &switchFaultstatus),

        // End of dictionary marker
        CO_OBJ_DICT_ENDMARK,
    };
};

}// namespace LVSS
#endif