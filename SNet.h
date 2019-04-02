//implementation of basic sensor network

#ifndef __SNET_H_INCLUDED__
#define __SNET_H_INCLUDED__

//#include <SNetConfig.h>

// these includes need to be in the main body of the arduino program as the library sub directorys are only included by the arduino IDE if they are in the main body of the program
#include <Arduino.h>
#include <SPI.h>
#include <SNetConfig.h>

#ifdef USE_NRF24
	#include <RF24.h>
	#include <RF24Network.h> //included for the header definition
#endif

#ifdef USE_RFM69
	#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
	#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/RFM69
	struct RF24NetworkHeader
	{
	  uint16_t from_node; /**< Logical address where the message was generated */
	  uint16_t to_node; /**< Logical address where the message is going */
	  uint16_t id; /**< Sequential message ID, incremented every time a new frame is constructed */
	  unsigned char type; /**< <b>Type of the packet. </b> 0-127 are user-defined types, 128-255 are reserved for system */
	  unsigned char reserved; /**< *Reserved for system use* */
		RF24NetworkHeader() {}
		RF24NetworkHeader(uint16_t _to, unsigned char _type = 0): to_node(_to), type(_type) {}
	};
#endif

#include <EEPROM.h>

//class RF24;
//class RF24Network;

#define SNET_VERSION "v3.2"
//v2.2 same as 2.1 but now includes updated RF24 and RF24Network libraries.
//v2.3 includes auto wake function after radio has been put to sleep
//v3.0 includes RFM69 compatibility - still needs testing!!
//v3.1 fixed memory leak in RFM69 code
//v3.2 RFM/NRF define now done in user space

#define SNET_MESSAGE_TYPE_OFFSET 64  //to account for ack packets

//sensor types:   nb there is no sensor 0 to avoid spurious commands
#define SNET_NODEDETAILS 1     //details of the node: command2: 1-Node type, 2-Node version, 3-LEAF/NODE, 4-SnetV, 5-config
#define SNET_NODEADDRESS 2     //data field is always network address (node_address).  See below for command2.  Network address for NRF network.  Protected and used internally
#define SNET_NODESTATUS 3     //Generic field used for hello world wake up messages etc
#define SNET_NODEID 4     //used for setting sensor_id.  Command2 is EEPROM verification code (currently 100) - will write ROM if and only if this matches.  If command2 is zero, it will return verification code
#define SNET_WRITEEEPROM 10
#define SNET_READEEPROM 11
#define SNET_COMMAND 20
#define SNET_STATE 21
#define SNET_STATUS 22
#define SNET_SWITCH 30
#define SNET_CONTACT 31
#define SNET_DIMMER 32
#define SNET_LIGHT 33
#define SNET_BUZZER 34
#define SNET_TEMPERATURE 50
#define SNET_HUMIDITY 51
#define SNET_MOISTURE 52
#define SNET_LIGHTLEVEL 53
#define SNET_VOLTAGE 70
#define SNET_CURRENT 71
#define SNET_POWER 72
#define SNET_ENERGY 73
#define SNET_POSITION 80
#define SNET_CAPACITY 90
#define SNET_VOLUME 91
#define SNET_FLOW 92
#define SNET_IRSEND 110
#define SNET_RCDATA 111     //for rc receive
#define SNET_RCSEND 112
#define SNET_RGB 113
#define SNET_RGBW 114
#define SNET_MILIGHT 115
#define SNET_TIME 130
#define SNET_TIMEREMAINING 131
#define SNET_INTERVAL 132
#define SNET_DIGITAL 140
#define SNET_ANALOGUE 141
#define SNET_COUNT 150
#define SNET_ADDRESS 151     //For any other sort of address - eg controller or dmx address



enum SNetEepromAddress {SNET_EEPROM_VERIFY=0,SNET_EEPROM_NODE_ADDRESS=1, SNET_EEPROM_NODE_ID=3};  //if this is changed, then you should change the verification code below
#define SNET_EEPROM_VERIFICATION_CODE 100
//NB first 5 bytes are reserved for SNET

struct payload_t
{
	uint16_t sensor_id;   //is normally used as from-sensor, unless coming from the bridge, in which case it is used as to-sensor
	uint16_t command1;
	uint16_t command2;
	union {
		float f;
		long l;
		byte b[4];  //uint8_t
		char c[4];   //int8_t
		//int16_t i[2];
	} data; // 4 bytes
};


class SNet {
private:
	#ifdef USE_NRF24
		RF24 radio;
		RF24Network network;
	#endif
	#ifdef USE_RFM69
		#ifdef ENABLE_ATC
		  RFM69_ATC radio;
		#else
		  RFM69 radio;
		#endif
		uint16_t _msg_count=0;  //RF24 this is taken care of automatically, but this has to be manually dealt with here
	#endif

	#ifdef USE_RFM69
		uint16_t node_address=2;  //default fall back
	#endif
	#ifdef USE_NRF24
		uint16_t node_address=1;
	#endif

	uint16_t sensor_id=100;

	#ifdef USE_RFM69
		uint16_t to_node=1;
	#endif
	#ifdef USE_NRF24
		uint16_t to_node=0;
	#endif

	bool sendGeneric(unsigned char type, uint16_t command1, uint16_t command2,  void * data); //internal use only - after type has been determined
	void loadEeprom();
	bool radio_asleep=false;
	bool write(RF24NetworkHeader * header_to_send, payload_t * payload_to_send);

public:
	SNet(uint8_t , uint8_t , uint16_t _node_address, uint16_t _sensor_id); //optional sensor id - can be set now if constant, or set at send time
	SNet(uint8_t , uint8_t ); //will load node_address and sensor_id from eeprom
				//CE , CSN for NRF24
				//CE , IRQ for RFM69
	~SNet();

	payload_t payload; // payload - used as last received
	RF24NetworkHeader header; //header - used as last received
	int16_t rssi; //only available if using RFM. Zero if using NRF. Also found in recieved copy of header.to_node

	void setEeprom();

	int device; //device to which the packet is being sent, not to be confused with _this_node
	void begin(bool load_eeprom);
	void update();
	//bool sendAll();

	bool sendFull(uint16_t _to_node, unsigned char type, payload_t * payload_to_send);

	//type bit pattern: LSB - always set. next two are data type, bit5 is command2 present
	bool send(uint16_t command1, float data); //type 1
	bool send(uint16_t command1, long data);  //type 3
 	bool send(uint16_t command1, byte data[4]); //type 5
	bool send(uint16_t command1, char data[4]); //type 7
	//bool send(uint16_t command1, int16_t data[2]); //type 9
	bool send(uint16_t command1, uint16_t command2,  float data); // type 17
	bool send(uint16_t command1, uint16_t command2,  long data); //type 19
	bool send(uint16_t command1, uint16_t command2,  byte data[4]); // type 21
	bool send(uint16_t command1, uint16_t command2,  char data[4]); // type 23
	//bool send(uint16_t command1, uint16_t command2,  int16_t data[2]); // type 25

	bool sendDetails(String sensor_type, String version_number,bool is_leaf_node);

	bool available;
	void sleep();
	void wake();

	void setNodeAddress(uint16_t _node_address);   //NB, this command needs to be followed by SNet.begin(); to take effect
	uint16_t getNodeAddress();
	void setSensorID(uint16_t _sensor_id);
	uint16_t getSensorID();
	void sendToNode(uint16_t _to_node);
	const char* toString(void) const; //very memery intensive  - don't know why.  memory leak?  crashes TRV

};


#endif // __SNET_H_INCLUDED__
