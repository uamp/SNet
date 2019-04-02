#ifndef __SNETCONF_H_INCLUDED__
#define __SNETCONF_H_INCLUDED__

#ifdef SNET_RFM69 //defined at user/program level
  #define USE_RFM69
#else
  #define USE_NRF24
#endif

#define SERIAL_BAUD   115200
#define NETWORK_ADDRESS 90
//#define SNET_WRITEPROTECT

//RFM SETUP   *******************************************************************************************************************
#ifdef USE_RFM69

  #ifndef FREQUENCY
    #define FREQUENCY     RF69_433MHZ
    //#define FREQUENCY     RF69_868MHZ
    //#define FREQUENCY     RF69_915MHZ
  #endif

  #define ENCRYPTKEY    "SNETEncrypt&1234" //exactly the same 16 characters/bytes on all nodes!
  #define IS_RFM69HW_HCW  true //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
  //#define ENABLE_ATC      //comment out this line to disable AUTO TRANSMISSION CONTROL
  #define ATC_RSSI        -75
  #define PROMISCUOUS_MODE true

#endif

//NRF SETUP   *******************************************************************************************************************
#ifdef USE_NRF24

#endif

#endif //__SNETCONF_H_INCLUDED__
