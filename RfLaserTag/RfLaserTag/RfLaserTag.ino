/*-----( Import needed libraries )-----*/
#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>
#include <RF24.h>
#include <IRremote.h>
#include "RfLaserTag.h"

/*-----( Declare Constants and Pin Numbers )-----*/
//#define LED_PIN 5
#define RECV_PIN 2
#define RF_IRQ_PIN 3
#define SD_CS_PIN 4
#define RF_CE_PIN 7
#define RF_CS_PIN 8
#define SPEAKER_PIN 9

#define SHOT_SOUND "01.wav"
#define EMPTY_SOUND "02.wav"
#define CLIP_SOUND "03.wav"
#define ACTION_SOUND "04.wav"
#define MISS_SOUND "05.wav"
//#define HIT_SOUND "06.wav"
#define KILL_SOUND "07.wav"
#define POWER_SOUND "08.wav"
#define SILENCED_SOUND "09.wav"

/*-----( Declare objects )-----*/
TMRpcm audio;   // create an object for use in this sketch
// (Create an instance of a radio, specifying the CE and CS pins. )
RF24 radio(RF_CE_PIN, RF_CS_PIN); // "radio" is the identifier you will use in following methods
IRrecv irrecv(RECV_PIN);


/*-----( Declare Variables )-----*/
decode_results results;
int dead = 0;
//int primaryInitSent = 0;
//int secondaryInitSent = 0;
uint8_t weponState[2] = {0, 0};
bool newIr = false;

void processIr() {
	if (irrecv.decode(&results)) {
#ifdef RF_LASER_DEBUG
		Serial.println(results.value, BIN);
    Serial.println(results.bits);
#endif
		if (results.bits == 14 && ((results.value >> 14) & 1) == 0) {
      unsigned int playerId = (results.value >> 6) & 127;
      unsigned short teamId = (results.value >> 4) & 3;
      unsigned short damage = results.value & 15;
#ifdef RF_LASER_DEBUG
     Serial.println(playerId, DEC);
     Serial.println(teamId, DEC);
     Serial.println(damage, HEX);
#endif
      audio.play(KILL_SOUND);
      dead++;
		} else {
       audio.play(MISS_SOUND);     
	  }
    irrecv.resume(); // Receive the next value
	}
  newIr = false;
}

void processMessage(int message, int pipe) {
  //char wavFile[33];
	switch (message) {
		case INIT_MESSAGE:
#ifdef RF_LASER_DEBUG
			Serial.print(F("got INIT_MESSAGE from pipe "));
      Serial.println(pipe);
#endif
			//if (pipe == PRIMARY_GUN) {
				if (!weponState[pipe - 1]) {
					weponState[pipe - 1] = 1;
				}
			//} else if (pipe == SECONDARY_GUN) {
			//	if (!weponState[pipe - 1]) {
		//			secondaryInitSent = 1;
			//	}
		//	}
      audio.play(ACTION_SOUND);
			break;
		case SHOT_MESSAGE:
			if (pipe == PRIMARY_GUN) {
#ifdef RF_LASER_DEBUG
				Serial.println(F("got primary SHOT_MESSAGE"));
#endif
        //strcpy_P(wavFile, wav_table[SHOT_SOUND]);
				audio.play(SHOT_SOUND);
			} else {// if (pipe == SECONDARY_GUN) {
#ifdef RF_LASER_DEBUG
				Serial.println(F("got secondery SHOT_MESSAGE"));
#endif
			 audio.play(SILENCED_SOUND);
			}
			break;
		case EMPTY_MESSAGE:
#ifdef RF_LASER_DEBUG
			Serial.println(F("got EMPTY_MESSAGE"));
#endif
			audio.play(EMPTY_SOUND);
			break;
		case RELOAD_START_MESSAGE:
#ifdef RF_LASER_DEBUG
			Serial.println(F("got RELOAD_START_MESSAGE"));
#endif
			audio.play(CLIP_SOUND);
			break;
		case RELOAD_END_MESSAGE:
#ifdef RF_LASER_DEBUG
			Serial.println(F("got primary RELOAD_END_MESSAGE"));
#endif
			audio.play(ACTION_SOUND);
			break;
		default:
#ifdef RF_LASER_DEBUG
			Serial.println(F("Message not recognized"));
#endif
			break;
			}
     //newRf = false;
}

void processRadio() {
	if (radio.available()) {
    byte dataReceived;  // Data that will be received from the transmitter
		byte pipeNo;
		while(radio.available(&pipeNo)) {
			radio.read(&dataReceived, 1);
		//  delay(1);
		}
   radio.writeAckPayload(pipeNo, &weponState[pipeNo - 1], sizeof(weponState[pipeNo - 1]));
//   if (pipeNo == PRIMARY_GUN) {
//    radio.writeAckPayload(pipeNo, &primaryInitSent, 1);
//   } else {
//      radio.writeAckPayload(pipeNo, &secondaryInitSent, 1);
//   }
#ifdef RF_LASER_DEBUG
		Serial.println(F("Data Received: "));
		Serial.println(dataReceived);
		Serial.println(F("On Pipe: "));
		Serial.println(pipeNo);
#endif
		processMessage(dataReceived, pipeNo);
	}
}

void setup()
{
#ifdef RF_LASER_DEBUG
	Serial.begin(115200);
#endif
	delay(500);
	audio.speakerPin = SPEAKER_PIN;
  audio.setVolume(5);
	SD.begin(SD_CS_PIN);

  //attachInterrupt(0,check_ir,CHANGE);
	// Start the IR receiver
	irrecv.enableIRIn(); 

	radio.begin();
	radio.setAutoAck(1);
	radio.enableAckPayload();                     // Allow optional ack payloads
  radio.setPayloadSize(1);
	//radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads
	radio.setChannel(RF24_CHANNEL);
	radio.setPALevel(RF24_PA_MIN);
	//radio.setPALevel(RF24_PA_MAX);  // Uncomment for more power
	//radio.setDataRate(RF24_2MBPS);
	radio.setRetries(5, 15);
	radio.openReadingPipe(PRIMARY_GUN, pipes[PRIMARY_GUN]);
	radio.openReadingPipe(SECONDARY_GUN, pipes[SECONDARY_GUN]);
	radio.startListening();
  radio.writeAckPayload(PRIMARY_GUN, &weponState[PRIMARY_GUN - 1], sizeof(weponState[PRIMARY_GUN - 1]));
  radio.writeAckPayload(SECONDARY_GUN, &weponState[SECONDARY_GUN - 1], sizeof(weponState[SECONDARY_GUN - 1]));
//  radio.writeAckPayload(PRIMARY_GUN, &primaryInitSent, 1);
//  radio.writeAckPayload(SECONDARY_GUN, &secondaryInitSent, 1);

 // attachInterrupt(0, check_radio, LOW); 
  
#ifdef RF_LASER_DEBUG
	Serial.println(F("Ready"));
	radio.printDetails();
#endif
  delay(500);
  audio.play(POWER_SOUND);
}

void loop() {
	if (!dead)
	{
    processRadio();
    //if (newIr) 
    processIr();
	}
	delay(100);//to sync things up VERY VERY importent does not work well with other values
}

//void check_ir(){
//  while(irrecv.decode(&results)){
//    irrecv.resume();
//  }
//  newIr = true;
//}

//void check_radio(void) 
//{
//  bool tx,fail,rx;
//  radio.whatHappened(tx,fail,rx);                     // What happened?
//
//  // If data is available, handle it accordingly
//  if ( rx ){
//    uint8_t dataReceived;
//    uint8_t pipeNo;
//    while(radio.available(&pipeNo)) {
//      radio.read(&dataReceived, sizeof(dataReceived));
//    //  delay(1);
//    }
//    radio.writeAckPayload(pipeNo, &weponState[pipeNo - 1], sizeof(weponState[pipeNo - 1]));
//    newRf = true;
//  }
//}
