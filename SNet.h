//implementation of basic sensor network

#ifndef __SNET_H_INCLUDED__
#define __SNET_H_INCLUDED__

// NB these includes need to be in the main body of the arduino program as the library sub directorys are only included by the arduino IDE if they are in the main body of the program
#include "Arduino.h"
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>

//class RF24;
//class RF24Network;

//sensor types:   nb there is no sensor 0 to avoid spurious commands
#define SNET_HOTTUB 1
#define SNET_RC_P1 2
#define SNET_HOMEEASY 3
#define SNET_DHT22 4
#define SNET_BLINDS 5
#define SNET_IR 6
#define SNET_ANLG 7 //sends a value 0-100%
#define SNET_DMX 8
#define SNET_RC_P2 9
#define SNET_GENERIC 10
#define SNET_RECEIVE 11
#define SNET_KETTLE 12
#define SNET_EEPROM_WRITE 13
#define SNET_EEPROM_READ 14

#define DHT_TEMP 1
#define DHT_HUMID 2
#define DHT_GETDATA 3

#define IR_ZEROEEPROM 0

#define RC_ON 1
#define RC_OFF 0
#define RC_DATA 20

//used by snet_generic
#define POWER 0
#define ENERGY 1
#define TEMPERATURE 3
#define VOLTAGE 4
#define T1 5
#define T2 6
#define HUMITIDY 7
#define FLOW 8

struct payload_t
{
	//char sensor[5];
	//long data;
	int sensor;
	int command;
	long data;
};


class SNet : public payload_t{
private:
	RF24 radio;
	RF24Network network;

	//payload_t payload;

	//RF24NetworkHeader header;
	// Address of our node
	
	
public:


	SNet(int ce_pin, int csn_pin, int this_node);
	~SNet();

	int device; //device to which the packet is being sent, not to be confused with _this_node
	void begin();
	void update();
	bool send();
	bool send(int device, int sensor, int command, long data);
	bool available;
	uint16_t mynode;// = 1;
	void sleep();
};	


#endif // __SNET_H_INCLUDED__ 


