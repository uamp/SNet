// NB these includes need to be in the main body of the arduino program
#include "Arduino.h"
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <SNet.h>


SNet::SNet(int ce_pin, int csn_pin, int this_node):radio(ce_pin,csn_pin),network(radio){
	mynode=(uint16_t)this_node;
}

void SNet::begin(){
	SPI.begin();
	radio.begin();
	radio.setAutoAck(true);
	network.begin( 90, mynode); //channel, node address
	radio.setDataRate(RF24_250KBPS); //needs to come after network begin to reset away from default
	radio.setPALevel(RF24_PA_HIGH);
	this->available=false;
}


//SNet::SNet(){//int ce_pin, int csn_pin, int this_node){
 // radio = new RF24(9,10);	
  //RF24 radio(9,10);
  //radio=radio2;
  //RF24Network network(radio);
 
//}


void SNet::update(){
	network.update();
	// Is there anything ready for us?
	if (network.available())
	{
		// If so, grab it
		RF24NetworkHeader header;
		payload_t payload;
		network.read(header,&payload,sizeof(payload));
		this->device=header.from_node;
		this->sensor=payload.sensor;
		this->command=payload.command;
		//memcpy(this->sensor, payload.sensor, sizeof(payload.sensor));
		this->data=payload.data;
		this->available=true;
	}
}

bool SNet::send(){
	if (device!=mynode){
		return send(this->device,this->sensor,this->command,this->data);
	} else {
		return false;  //stops it trying to send a message to itself
	}
}

bool SNet::send(int device, int sensor, int command, long data){
	  payload_t payload;  
	  payload.sensor=sensor;
	  payload.command=command;
	  payload.data=data;
      	  RF24NetworkHeader header(device); //to node
      	  bool ok = network.write(header,&payload,sizeof(payload));
	  return ok;
}

void SNet::sleep(){
	radio.powerDown();
}


SNet::~SNet(){
}
