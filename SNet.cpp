// these includes need to be in the main body of the arduino program as the library sub directorys are only included by the arduino IDE if they are in the main body of the program
#include <Arduino.h>
#include <SNetConfig.h>
#include <SPI.h>
#include <EEPROM.h>
#include <SNet.h>

#ifdef USE_NRF24
SNet::SNet(uint8_t ce_pin, uint8_t csn_pin, uint16_t _node_address, uint16_t _sensor_id):radio(ce_pin,csn_pin),network(radio){
	rssi=0;
	node_address=_node_address;
	sensor_id=_sensor_id;
	//to_node=0;
	//this->begin(); - this does not work when put here
}
SNet::SNet(uint8_t ce_pin, uint8_t csn_pin):radio(ce_pin,csn_pin),network(radio){
	//to_node=0;
}
#endif
#ifdef USE_RFM69
SNet::SNet(uint8_t slaveSelectPin, uint8_t interruptPin, uint16_t _node_address, uint16_t _sensor_id):radio(slaveSelectPin,interruptPin,IS_RFM69HW_HCW){
	//RFM69(uint8_t slaveSelectPin=RF69_SPI_CS, uint8_t interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false);
  rssi=0;
	node_address=_node_address;
	sensor_id=_sensor_id;
	//to_node=0; //1 should be the default?
	//this->begin(); - this does not work when put here
}
SNet::SNet(uint8_t slaveSelectPin, uint8_t interruptPin):radio(slaveSelectPin,interruptPin,IS_RFM69HW_HCW){
	//to_node=0;
}
#endif


void SNet::begin(bool load_eeprom=false){
	if(load_eeprom) {
		this->loadEeprom();
	}
	this->wake();
	SPI.begin();
	#ifdef USE_NRF24
		radio.begin();
		radio.setPayloadSize((sizeof(RF24NetworkHeader)+sizeof(payload_t)));  //consider selecting dynamic payloads here or setting payload length - need to check RF24network doesn't set this though
		//radio.setPALevel(RF24_PA_HIGH); //defaults to RF24_PA_MAX on begin() anyway
		network.begin( NETWORK_ADDRESS , node_address); //channel, node address
		radio.setAutoAck(true); // set (0,0) in network.begin which means not on pipe 0. - this should probably go after the network begin statement.
		//radio.setDataRate(RF24_250KBPS); //needs to come after network begin to reset away from default - need to check this works on the chips with PA.
					//it check p/+ variant in radio module (not network) and then resets to 1mpbs if it is.  will ignore for now and use 1mbs
		//radio.setPALevel(RF24_PA_LOW);
	#endif

	#ifdef USE_RFM69
		radio.initialize(FREQUENCY,node_address,NETWORK_ADDRESS);
		#ifdef IS_RFM69HW_HCW
			radio.setHighPower(); //must include this only for RFM69HW/HCW!
		#endif
		radio.encrypt(ENCRYPTKEY);
		radio.promiscuous(PROMISCUOUS_MODE);
		#ifdef ENABLE_ATC
			radio.enableAutoPower(ATC_RSSI);
		#endif
	#endif

	this->available=false;

}

void SNet::loadEeprom(){
	if(EEPROM.read(SNET_EEPROM_VERIFY)==SNET_EEPROM_VERIFICATION_CODE) {
		EEPROM.get(SNET_EEPROM_NODE_ADDRESS,node_address);
		EEPROM.get(SNET_EEPROM_NODE_ID,sensor_id);
	}
	//this->begin();
}

void SNet::setEeprom(){
	#ifndef SNET_WRITEPROTECT
		EEPROM.put(SNET_EEPROM_NODE_ADDRESS,node_address);
		EEPROM.put(SNET_EEPROM_NODE_ID,sensor_id);
		EEPROM.write(SNET_EEPROM_VERIFY,SNET_EEPROM_VERIFICATION_CODE);
	#endif
}

void SNet::setNodeAddress(uint16_t _node_address){
	node_address=_node_address;
}

uint16_t SNet::getNodeAddress(){
	return node_address;
}

void SNet::sendToNode(uint16_t _to_node){
	to_node=_to_node;
}

void SNet::setSensorID(uint16_t _sensor_id){
	sensor_id=_sensor_id;
}

uint16_t SNet::getSensorID(){
	return sensor_id;
}

void SNet::update(){
	//this->wake();
	#ifdef USE_NRF24
		network.update();
		// Is there anything ready for us?
		if (network.available()){
			// If so, grab it
			network.read(header,&payload,sizeof(payload));
	#endif  //NRF24
	#ifdef USE_RFM69
		if(radio.receiveDone()){
			//Serial.println("here1");
			//if(radio.DATALEN==(sizeof(payload_t)+sizeof(RF24NetworkHeader))){

				//memcpy(&header,&(char *)radio.DATA,sizeof(RF24NetworkHeader));
				//uint8_t & data_addr=const_cast<uint8_t &>(radio.DATA);
				//memcpy(&header,radio.DATA,sizeof(RF24NetworkHeader));
				//memcpy(&payload,&radio.DATA + sizeof(RF24NetworkHeader),sizeof(payload_t));
				//memcpy cannot work as radio.DATA is volatile, and memcpy cannot work on a volatile obj
				//(https://stackoverflow.com/questions/36729240/passing-argument-2-of-memcpy-discards-volatile-qualifier-from-pointer-target)
				uint8_t temp_byte;
				uint8_t rec_buffer[sizeof(RF24NetworkHeader)+sizeof(payload_t)];
				/*
				Serial.print("Start [");
				Serial.print(radio.DATALEN);
				Serial.print("]");
				*/
				for (byte i = 0; i < radio.DATALEN; i++){
					temp_byte=radio.DATA[i];
					//Serial.print(temp_byte);
					//Serial.print(" ");
					rec_buffer[i]=temp_byte;
				}
				//Serial.println(" - end");
				memcpy(&header,&rec_buffer,sizeof(RF24NetworkHeader));
				memcpy(&payload,&rec_buffer[sizeof(RF24NetworkHeader)],sizeof(payload_t));

				header.to_node=radio.RSSI; //to_node "shouldn't" be needed once packet has successfully arrived at the correct destination
																		//caveat to this is when promiscuous mode is on and it is sniffing all packets
																		//note implicit conversion from int16_t to uint16_t
				rssi=radio.RSSI; //might be worth deleting line above if not needed in openhab/nodered
				//debugging!
				/*
				Serial.print(F("Header: "));
				Serial.print(header.from_node);
				Serial.print(F("-"));
				Serial.print((int16_t)header.to_node);
				Serial.print(F("-"));
				Serial.print(header.type);
				Serial.print(F("-"));
				Serial.print(header.id);
				Serial.println();

				Serial.print(F("Packet: "));
				Serial.print(payload.sensor_id);
				Serial.print(F("-"));
				Serial.print(payload.command1);
				Serial.print(F("-"));
				Serial.print(payload.data.l);
				Serial.println();
				*/
			//}
			if (radio.ACKRequested())
			{
				radio.sendACK();
			}
	#endif //USE_RFM69
			header.type=header.type-SNET_MESSAGE_TYPE_OFFSET;
			this->available=true;
			//this->device=header.from_node;
			//this->sensor=payload.sensor;
			//this->command=payload.command;
			//memcpy(this->sensor, payload.sensor, sizeof(payload.sensor));
			//this->data=payload.data;

			//next block allows re-allocation of network id on the fly
			/* if(payload.sensor_id==this->sensor_id){
				this->available=true;
				if(payload.command1==SNET_NODEADDRESS || payload.command1==SNET_NODEID ){
					if(payload.command1==SNET_NODEADDRESS) setNodeAddress((uint16_t)payload.data.l);
					if(payload.command1==SNET_NODEID) setSensorID((uint16_t)payload.data.l);
					if((header.type & 0x10)==0x10) { //ensure bit set for command2
						if (payload.command2==SNET_EEPROM_VERIFICATION_CODE) {
							setEeprom();    //write EEPROM if command2 is correct verification code
							send(SNET_NODEADDRESS,0,(long)SNET_EEPROM_VERIFICATION_CODE);    //return a value to indicate success and to force update at main
						}
						if (payload.command2==0) send(SNET_NODEADDRESS,0,(long)SNET_EEPROM_VERIFICATION_CODE);	//return verification code to the requester
					}
					begin();  //reset network - not sure I want to do this with RFM69
					this->available=false; //does this anyway in begin, but good to make sure			//no need to return availability to main program
				}
			}
			if(getSensorID()==0) this->available=true; //ensure base station passes on messages regardless */

		}

}

bool SNet::sendDetails(char sensor_type[4],char version_number[4],bool is_leaf_node){
	this->wake();
	/*
	//this needs modifying as it doesn't work on ESP
	bool ok= send(SNET_NODEDETAILS,1,sensor_type);
	ok &=  send(SNET_NODEDETAILS,2,version_number);
	if(is_leaf_node) {
		ok &=  send(SNET_NODEDETAILS,3,"LEAF");
	} else {
		ok &=  send(SNET_NODEDETAILS,3,"NODE");
	}
	ok &=  send(SNET_NODEDETAILS,4,SNET_VERSION);
	return ok;
	*/
	return true;
}


bool SNet::write(RF24NetworkHeader * header_to_send, payload_t * payload_to_send){
	//this->wake();
	#ifdef USE_NRF24
		bool ok = network.write(header_to_send,&payload_to_send,sizeof(payload_t));
	#endif
	#ifdef USE_RFM69
		//uint8_t buffer[RF69_MAX_DATA_LEN];
		uint8_t buffer[sizeof(RF24NetworkHeader)+sizeof(payload_t)];
		header_to_send->id=_msg_count++;  //dealt with automatically for NRF24
		header_to_send->from_node=node_address;
		memcpy(&buffer,header_to_send,sizeof(RF24NetworkHeader));
		memcpy(&buffer[sizeof(RF24NetworkHeader)],payload_to_send,sizeof(payload_t));
		//memcpy(&buffer,payload_to_send,sizeof(payload_t));
		bool ok = radio.sendWithRetry(header_to_send->to_node, buffer, sizeof(RF24NetworkHeader)+sizeof(payload_t));

		//debugging!
		/*
		Serial.print(F("Header: "));
		Serial.print(header_to_send->to_node);
		Serial.print(F("-"));
		Serial.print(header_to_send->type);
		Serial.print(F("-"));
		Serial.print(header_to_send->id);
		Serial.println();

		Serial.print(F("Packet: "));
		Serial.print(payload_to_send->sensor_id);
		Serial.print(F("-"));
		Serial.print(payload_to_send->command1);
		Serial.print(F("-"));
		Serial.print(payload_to_send->data.l);
		Serial.println();

		Serial.print(F("Start: "));
		for(int i=0;i<(sizeof(RF24NetworkHeader)+sizeof(payload_t));i++){
			Serial.print(buffer[i]);
			Serial.print(" ");
		}
		Serial.println(F(" end"));
		delay(10); */

	#endif
	return ok;
}

bool SNet::sendFull(uint16_t _to_node, unsigned char type, payload_t * payload_to_send){
	this->wake();
	if (_to_node==node_address){
		return false;  //stops it trying to send a message to itself
	} else {
		RF24NetworkHeader _header(_to_node,type); //to node
		bool ok=this->write(&_header,payload_to_send);
		return ok;
	}

}

bool SNet::send(uint16_t command1, float data){ //type 1
	/* this->wake();
	RF24NetworkHeader _header(to_node,1+SNET_MESSAGE_TYPE_OFFSET);
	payload_t payload_to_send;
	payload_to_send.sensor_id=sensor_id;
	payload_to_send.command1=command1;
	payload_to_send.command2=0;
	payload_to_send.data.f=data;
	bool ok = network.write(_header,&payload_to_send,sizeof(payload_to_send));
	return ok;*/
	sendGeneric(1,command1,0,&data); // in testing
}


bool SNet::send(uint16_t command1, long data){  //type 3
	this->wake();
	RF24NetworkHeader _header(to_node,3+SNET_MESSAGE_TYPE_OFFSET);
	payload_t payload_to_send;
	payload_to_send.sensor_id=sensor_id;
	payload_to_send.command1=command1;
	payload_to_send.command2=0;
	payload_to_send.data.l=data;
	bool ok=this->write(&_header,&payload_to_send);
	return ok;
}

bool SNet::send(uint16_t command1, byte data[4]){ //type 5
	this->wake();
	RF24NetworkHeader _header(to_node,5+SNET_MESSAGE_TYPE_OFFSET);
	payload_t payload_to_send;
	payload_to_send.sensor_id=sensor_id;
	payload_to_send.command1=command1;
	payload_to_send.command2=0;
	for(byte i=0;i<4;i++) payload_to_send.data.b[i]=data[i];
	bool ok=this->write(&_header,&payload_to_send);
	return ok;
}

bool SNet::send(uint16_t command1, char data[4]){ //type 7
	this->wake();
	RF24NetworkHeader _header(to_node,7+SNET_MESSAGE_TYPE_OFFSET);
	payload_t payload_to_send;
	payload_to_send.sensor_id=sensor_id;
	payload_to_send.command1=command1;
	payload_to_send.command2=0;
	for(byte i=0;i<4;i++) payload_to_send.data.c[i]=data[i];
	bool ok=this->write(&_header,&payload_to_send);
	return ok;
}


bool SNet::send(uint16_t command1, uint16_t command2,  float data){ // type 17
	this->wake();
	RF24NetworkHeader _header(to_node,17+SNET_MESSAGE_TYPE_OFFSET);
	payload_t payload_to_send;
	payload_to_send.sensor_id=sensor_id;
	payload_to_send.command1=command1;
	payload_to_send.command2=0;
	payload_to_send.data.f=data;
	bool ok=this->write(&_header,&payload_to_send);
	return ok;
}

bool SNet::send(uint16_t command1, uint16_t command2,  long data){ //type 19
	this->wake();
	RF24NetworkHeader _header(to_node,19+SNET_MESSAGE_TYPE_OFFSET);
	payload_t payload_to_send;
	payload_to_send.sensor_id=sensor_id;
	payload_to_send.command1=command1;
	payload_to_send.command2=command2;
	payload_to_send.data.l=data;
	bool ok=this->write(&_header,&payload_to_send);
	return ok;
}

bool SNet::send(uint16_t command1, uint16_t command2,  byte data[4]){ // type 21
	this->wake();
	RF24NetworkHeader _header(to_node,21+SNET_MESSAGE_TYPE_OFFSET);
	payload_t payload_to_send;
	payload_to_send.sensor_id=sensor_id;
	payload_to_send.command1=command1;
	payload_to_send.command2=command2;
	for(byte i=0;i<4;i++) payload_to_send.data.b[i]=data[i];
	bool ok=this->write(&_header,&payload_to_send);
	return ok;
}

bool SNet::send(uint16_t command1, uint16_t command2,  char data[4]){ // type 23
	/* this->wake();
	RF24NetworkHeader _header(to_node,23+SNET_MESSAGE_TYPE_OFFSET);
	payload_t payload_to_send;
	payload_to_send.sensor_id=sensor_id;
	payload_to_send.command1=command1;
	payload_to_send.command2=command2;
	for(byte i=0;i<5;i++) payload_to_send.data.c[i]=data[i];
	bool ok = network.write(_header,&payload_to_send,sizeof(payload_to_send));
	return ok;*/
	sendGeneric(23,command1,command2,data); //currently under testing
}

bool SNet::sendGeneric(unsigned char type, uint16_t command1, uint16_t command2,  void * data){  //experimental
	this->wake();
	RF24NetworkHeader _header(to_node,type+SNET_MESSAGE_TYPE_OFFSET);
	payload_t payload_to_send;
	payload_to_send.sensor_id=sensor_id;
	payload_to_send.command1=command1;
	payload_to_send.command2=command2;
	memcpy(&payload_to_send.data,data,4); //does payload_to_send.data have an address?
	//payload.data=data;
	bool ok=this->write(&_header,&payload_to_send);
	return ok;
}


void SNet::sleep(){
	#ifdef USE_NRF24
		radio.powerDown();
	#endif
	#ifdef USE_RFM69
		radio.sleep();
	#endif
	radio_asleep=true;
}

void SNet::wake(){
	if(radio_asleep){
		#ifdef USE_NRF24
			radio.powerUp();
			delay(5); //see RF24.h
		#endif
		//RFM should wake once it's told to transmit!?
		radio_asleep=false;

	}
}

const char* SNet::toString(void) const {
  static char buffer[60];
  byte option=(header.type - SNET_MESSAGE_TYPE_OFFSET )& 0x0F;
  //snprintf_P(buffer,sizeof(buffer),PSTR("id %04x from 0%o to 0%o type %c"),id,from_node,to_node,type);
  if(option==5){
	snprintf_P(buffer,sizeof(buffer),PSTR("D: %d %d %d %d %d %d - %02X %02X %02X %02X :E"),header.from_node,header.id,header.type,payload.sensor_id,payload.command1,payload.command2,payload.data.b[0],payload.data.b[1],payload.data.b[2],payload.data.b[3]);
  } else if (option==7) {
	snprintf_P(buffer,sizeof(buffer),PSTR("D: %d %d %d %d %d %d - %c%c%c%c :E"),header.from_node,header.id,header.type,payload.sensor_id,payload.command1,payload.command2,payload.data.c[0],payload.data.c[1],payload.data.c[2],payload.data.c[3]);
  } else if (option==3) {
	snprintf_P(buffer,sizeof(buffer),PSTR("D: %d %d %d %d %d %d - %d :E"),header.from_node,header.id,header.type,payload.sensor_id,payload.command1,payload.command2,payload.data.l);
  } else if (option==1) {
	snprintf_P(buffer,sizeof(buffer),PSTR("D: %d %d %d %d %d %d - float :E"),header.from_node,header.id,header.type,payload.sensor_id,payload.command1,payload.command2); //float %f unsupported by arduino for code size reasons
  }
  return buffer;
}


SNet::~SNet(){
}
