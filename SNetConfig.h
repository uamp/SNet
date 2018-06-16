#ifndef __SNETCONF_H_INCLUDED__
#define __SNETCONF_H_INCLUDED__

#define USE_RFM69
//#define USE_NRF24

#define SERIAL_BAUD   115200
#define NETWORK_ADDRESS 90

//RFM SETUP   *******************************************************************************************************************
#ifdef USE_RFM69
  //Match frequency to the hardware version of the radio on your Moteino (uncomment one):
  //#define FREQUENCY     RF69_433MHZ
  #define FREQUENCY     RF69_868MHZ
  //#define FREQUENCY     RF69_915MHZ
  #define ENCRYPTKEY    "SNETEncrypt&1234" //exactly the same 16 characters/bytes on all nodes!
  #define IS_RFM69HW_HCW  true //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
  #define ENABLE_ATC      //comment out this line to disable AUTO TRANSMISSION CONTROL
  #define ATC_RSSI        -75

#endif

//NRF SETUP   *******************************************************************************************************************
#ifdef USE_NRF24

#endif

#endif //__SNETCONF_H_INCLUDED__
