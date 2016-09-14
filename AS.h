//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <trilu@gmx.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef _NAS_H
#define _NAS_H

#include "cmMaster.h"
#include "message_union.h"
#include "Defines.h"
#include "HAL.h"
#include "CC1101.h"
#include "EEprom.h"
#include "Send.h"
#include "Receive.h"
#include "ConfButton.h"
#include "StatusLed.h"
#include "Power.h"
#include "Battery.h"
#include "Version.h"
#include "macros.h"

//const uint8_t empty_4_byte[] = { 0,0,0,0, };					// need it all time to get an empty peer slot or for compare... 
//#define EMPTY4BYTE (uint8_t*)empty_4_byte


/**
 * @short Main class for implementation of the AskSin protocol stack.
 * Every device needs exactly one instance of this class.
 *
 * AS is responsible for maintaining the complete infrastructure from
 * device register representation, non-volatile storage of device configuration
 * in the eeprom, to providing interfaces for user modules that implement
 * actual device logic.
 *
 * This is a very simple, non-working example of the basic API used in the main
 * routines:
 * @include docs/snippets/basic-AS.cpp
 *
 * All send functions are used by sensor or actor classes like THSensor or Dimmer.
 */


class AS {
	//friend class SN;
	//friend class RV;
	//friend class PW;

  public:		//---------------------------------------------------------------------------------------------------------
	//EE ee;				///< eeprom module
	//CC cc;				///< load communication module
	//SN sn;				///< send module
	//RV rv;				///< receive module
	//LD ld;				///< status led
	//CB confButton;		///< config button
	//BT bt;				///< battery status
	//PW pw;				///< power management
	//cmMaster *pcnlModule[];


	/** @brief Helper structure for keeping track of active config mode */
	struct s_confFlag {						// - remember that we are in config mode, for config start message receive
		uint8_t  active;					//< indicates status, 1 if config mode is active
		uint8_t  channel;					//< channel
		uint8_t  list;						//< list
		uint8_t  idx_peer;					//< peer index
	} cFlag;

	struct s_stcSlice {						// - send peers or reg in slices, store for send slice function
		uint8_t active;						// indicates status of poll routine, 1 is active
		uint8_t peer;						// is it a peer list message
		uint8_t reg2;						// or a register send
		uint8_t reg3;						// not implemented at the moment
		uint8_t totSlc;						// amount of necessary slices to send content
		uint8_t curSlc;						// counter for slices which are already send
		uint8_t cnl;						// indicates channel
		uint8_t lst;						// the respective list
		uint8_t idx;						// the peer index
		uint8_t mCnt;						// the message counter
		uint8_t toID[3];					// to whom to send
	} stcSlice;

	struct s_stcPeer {
		uint8_t active; //   :1;			// indicates status of poll routine, 1 is active
		uint8_t retries; //    :3;			// send retries
		uint8_t burst; //    :1;			// burst flag for send function
		uint8_t bidi; //     :1;			// ack required
		uint8_t msg_type;					// message type to build the right message
		uint8_t *ptr_payload;				// pointer to payload
		uint8_t len_payload;				// length of payload
		uint8_t channel;					// which channel is the sender
		uint8_t idx_cur;					// current peer slots
		uint8_t idx_max;					// amount of peer slots
		uint8_t slot[8];					// slot measure, all filled in a first step, if ACK was received, one is taken away by slot
	} stcPeer;

	union {
		struct s_l4_0x01 {
			uint8_t  peerNeedsBurst:1;			// 0x01, s:0, e:1
			uint8_t  :6;
			uint8_t  expectAES:1;				// 0x01, s:7, e:8
		} s;
		uint8_t	ui;
	} l4_0x01;

	uint8_t pairActive;

	uint8_t keyPartIndex = AS_STATUS_KEYCHANGE_INACTIVE;

	uint8_t  signingRequestData[6];
	uint8_t  tempHmKey[16];
	uint8_t  newHmKey[16];
	uint8_t  newHmKeyIndex[1];
	uint16_t randomSeed = 0;
	uint8_t  resetStatus = 0;

  public:		//---------------------------------------------------------------------------------------------------------
	AS();

	/**
	 * @brief Initialize the AS module
	 *
	 * init() has to be called from the main setup() routine.
	 */
	void init(void);

	/**
	 * @brief Poll routine for regular operation
	 *
	 * poll() needs to be called regularily from the main loop(). It takes care of
	 * all major tasks like sending and receiving messages, device configuration
	 * and message delegation.
	 */
	void poll(void);

	// - send functions --------------------------------
	void sendDEVICE_INFO(void);
	void sendACK(void);
	void checkSendACK(uint8_t ackOk);
	void sendPayload(uint8_t payloadType, uint8_t *data, uint8_t dataLen);
	inline void sendAckAES(uint8_t *data);
	void sendACK_STATUS(uint8_t channel, uint8_t state, uint8_t action);
	inline void sendNACK(void);
	void sendNACK_TARGET_INVALID(void);
	void sendINFO_ACTUATOR_STATUS(uint8_t channel, uint8_t state, uint8_t flag);
	void sendINFO_POWER_EVENT(uint8_t *data);
	void sendINFO_TEMP(void);
	void sendHAVE_DATA(void);
	void sendSWITCH(void);
	void sendTimeStamp(void);
	void sendREMOTE(uint8_t channel, uint8_t *ptr_payload, uint8_t msg_flag = 0);
	void sendSensor_event(uint8_t channel, uint8_t burst, uint8_t *payload);
	void sendSensorData(void);
	void sendClimateEvent(void);
	void sendSetTeamTemp(void);
	void sendWeatherEvent(void);
	void sendEvent(uint8_t channel, uint8_t msg_type, uint8_t msg_flag, uint8_t *ptr_payload, uint8_t len_payload);

	void processMessageConfigAction(uint8_t by10, uint8_t cnl1);
	void processMessageAction11();
	void processMessageAction3E(uint8_t cnl, uint8_t pIdx);
	void deviceReset(uint8_t clearEeprom);

	uint8_t getChannelFromPeerDB(uint8_t *pIdx);

	void initPseudoRandomNumberGenerator();



  //private:		//---------------------------------------------------------------------------------------------------------

	inline void processMessageSwitchEvent();

	inline void processMessageResponseAES_Challenge(void);
	inline void processMessageResponseAES(void);
	inline void processMessageKeyExchange(void);
	uint8_t checkAnyChannelForAES(void);

	uint8_t processMessageConfig();
	inline void processMessageConfigStatusRequest(uint8_t by10);
	inline void processMessageConfigPairSerial(void);
	inline void processMessageConfigSerialReq(void);
	inline void processMessageConfigParamReq(void);
	inline void processMessageConfigPeerListReq(void);
	inline void processMessageConfigAESProtected();

	inline void actionSwitchEvent();
	inline uint8_t configPeerAdd();
	inline uint8_t configPeerRemove();
	inline void configStart();
	inline void configEnd();
	inline void configWriteIndex(void);

	// - poll functions --------------------------------
	inline void sendSliceList(void);															// scheduler to send config messages, peers and regs
	inline void sendPeerMsg(void);																// scheduler for peer messages
	void preparePeerMessage(uint8_t *xPeer, uint8_t retr);
			
	// - receive functions -----------------------------
	void processMessage(void);

	// - send functions --------------------------------
	inline void sendINFO_SERIAL(void);
	inline void sendINFO_PEER_LIST(uint8_t len);
	inline void sendINFO_PARAM_RESPONSE_PAIRS(uint8_t len);
	void sendINFO_PARAM_RESPONSE_SEQ(uint8_t len);
	void sendINFO_PARAMETER_CHANGE(void);

	void prepareToSend(uint8_t mCounter, uint8_t mType, uint8_t *receiverAddr);

	// - AES Signing related methods -------------------
	void makeTmpKey(uint8_t *challenge);
	void payloadEncrypt(uint8_t *encPayload, uint8_t *msgToEnc);

	void sendSignRequest(uint8_t rememberBuffer);

	inline void initRandomSeed();
	
  protected:	//---------------------------------------------------------------------------------------------------------
	// - homematic specific functions ------------------
	void decode(uint8_t *buf);																// decodes the message
	void encode(uint8_t *buf);																// encodes the message
	void explainMessage(uint8_t *buf);														// explains message content, part of debug functions

	// - some helpers ----------------------------------


};

extern const uint8_t devIdnt[];
extern const uint8_t cnlAddr[];
/**
* @brief Array with channel defaults. Index and length are hold in the channel table array.
*        Must be declared in user space.
*/
extern const uint8_t cnlDefs[];
extern const uint8_t cnl_max;
extern const uint8_t cnl_tbl_max;


//extern uint8_t rcv_PEER_CNL;															// if message comes from a registered peer, rv class stores the registered channel here, declaration in AS.cpp
//extern uint8_t rcv_PEER_ID[];															// if message comes from a registered peer, rv class stores the peer id with cleaned peer channel here, declaration in AS.cpp
//extern u_Message rcv;																	// union for the received string
//extern u_Message snd;																	// union for the send string

extern AS hm;



/**
 * @short Timer class for non-blocking delays
 *
 * The following examples shows how to use the waitTimer class to
 * perform an action every 500ms. Note that the first time loop()
 * is called, delay.done() will return true and the action will
 * be performed. The example also shows how to avoid the execution
 * time of the action to influence the new delay time by setting 
 * the delay before performing the action.
 * @code
 * void loop()
 * {
 *     static waitTimer delay;
 *     if (delay.done()) {
 *         delay.set(500); // perform next action after 500ms
 *         // do something now
 *     }
 * }
 * @endcode
 *
 * @attention The waitTimer can only make sure a minimum time passes
 * between set() and done(). If calls to done() are delayed due to other
 * actions, more time may pass. Also, actual delay times strongly depend 
 * on the behaviour of the system clock.
 *
 * @see http://www.gammon.com.au/forum/?id=12127
 */
class waitTimer {

  private:		//---------------------------------------------------------------------------------------------------------
	uint8_t  armed;
	uint32_t checkTime;
	uint32_t startTime;

  public:		//---------------------------------------------------------------------------------------------------------
	uint8_t  done(void);
	void     set(uint32_t ms);
	uint32_t remain(void);
};

uint32_t byteTimeCvt(uint8_t tTime);
uint32_t intTimeCvt(uint16_t iTime);

#endif

