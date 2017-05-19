//implementation of basic sensor network

#ifndef __SNET_H_INCLUDED__
#define __SNET_H_INCLUDED__

// NB these includes need to be in the main body of the arduino program as the library sub directorys are only included by the arduino IDE if they are in the main body of the program
#include "Arduino.h"
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <EEPROM.h>

//class RF24;
//class RF24Network;

#define SNET_VERSION "v2.3"  
//v2.2 same as 2.1 but now includes updated RF24 and RF24Network libraries. 
//v2.3 includes auto wake function after radio has been put to sleep

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
	RF24 radio;
	RF24Network network;
	uint16_t node_address=1;  //default fall back
	uint16_t sensor_id=100; 
	uint16_t to_node=0;
	bool sendGeneric(unsigned char type, uint16_t command1, uint16_t command2,  void * data); //internal use only - after type has been determined
	void loadEeprom();
	bool radio_asleep=false;	

public:
	SNet(uint8_t ce_pin, uint8_t csn_pin, uint16_t _node_address, uint16_t _sensor_id); //optional sensor id - can be set now if constant, or set at send time
	SNet(uint8_t ce_pin, uint8_t csn_pin); //will load node_address and sensor_id from eeprom
	~SNet();

	payload_t payload; // payload - used as last received
	RF24NetworkHeader header; //payload - used as last received 

	void setEeprom();

	int device; //device to which the packet is being sent, not to be confused with _this_node
	void begin(bool load_eeprom);
	void update();
	//bool sendAll();

	bool sendFull(uint16_t _to_node, unsigned char type, payload_t payload_to_send);

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

	bool sendDetails(char sensor_type[4],char version_number[4],bool is_leaf_node);

	bool available;
	void sleep();
	void wake();
	
	void setNodeAddress(uint16_t _node_address);   //NB, this command needs to be followed by SNet.begin(); to take effect
	uint16_t getNodeAddress();
	void setSensorID(uint16_t _sensor_id);
	uint16_t getSensorID();
	void sendToNode(uint16_t _to_node);
	const char* toString(void) const;	

};	


#endif // __SNET_H_INCLUDED__ 

