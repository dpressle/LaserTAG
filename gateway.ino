  /**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 * The ArduinoGateway prints data received from sensors on the serial link. 
 * The gateway accepts input on seral which will be sent out on radio network.
 *
 * The GW code is designed for Arduino Nano 328p / 16MHz
 *
 * Wire connections (OPTIONAL):
 * - Inclusion button should be connected between digital pin 3 and GND  
 * - RX/TX/ERR leds need to be connected between +5V (anode) and digital pin 6/5/4 with resistor 270-330R in a series
 *
 * LEDs (OPTIONAL):
 * - To use the feature, uncomment MY_LEDS_BLINKING_FEATURE in MyConfig.h
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error 
 * 
 */

// Enable debug prints to serial monitor
#define MY_DEBUG 


// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level. 
#define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enable serial gateway
#define MY_GATEWAY_SERIAL

#define CHILD_ID 0
// Define a lower baud rate for Arduino's running on 8 MHz (Arduino Pro Mini 3.3V & SenseBender)
#if F_CPU == 8000000L
#define MY_BAUD_RATE 38400
#endif


#include <SPI.h>
#include <MySensors.h>  
#include <pcmConfig.h>
#include <pcmRF.h>
#include <TMRpcm.h>
#include <MyLaserTag.h>

#define SPEAKER_PIN 9
#define SD_CS_PIN 4

TMRpcm tmrpcm;   // create an object for use in this sketch

MyMessage commandMessage(CHILD_ID, V_COMMAND);
MyMessage systemMessage(CHILD_ID, V_SYSTEM);

void setup() { 
  // Setup locally attached sensors
  tmrpcm.speakerPin = SPEAKER_PIN;
  SD.begin(SD_CS_PIN);
  tmrpcm.volume(7);

}

void presentation() {
 // Present locally attached sensors 
}

void receive(const MyMessage &message) {
  switch (message.type) {
	case V_GUN:
	  int messagePayload = message.getInt();
	  switch (messagePayload) {
	    case FIRE_MESSAGE:
		  break;
	    case EMPTY_MESSAGE:
		  break;
		case RELOAD_START_MESSAGE:
		  break;
		case RELOAD_END_MESSAGE:
		  break;
		default:
          break;
      }
	case V_SENSOR:
	  int messagePayload = message.getInt();
	  switch (messagePayload) {
	    case HIT_MESSAGE:
		//play hit sound and handle the hit
		  break;
		case MISS_MESSAGE:
		//play miss sound
		  break;
		default:
          break;
      }
    default:
	  break;
  }
  return;
}

void loop() { 
  // Send locally attached sensor data here 
}

