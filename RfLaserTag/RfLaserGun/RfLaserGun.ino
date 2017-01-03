/*-----( Import needed libraries )-----*/
#include <SPI.h>
#include <RF24.h>
//#include "IRMilesTag.h"
#include <Bounce2.h>
#include <EEPROM.h>
//#include "RfLaserTag.h"
#include "RfLaserRifle.h"
//#include "RfLaserPistole.h"

/*-----( Declare Constants and Pin Numbers )-----*/
#define TRIGER_PIN 5
#define IR_PIN 3
#define RELOAD_PIN 4
#define CE_PIN 9
#define CS_PIN 10

#define EEPROM_AMMO 1
#define TOPBIT 0x80000000

/*-----( Declare objects )-----*/
// (Create an instance of a radio, specifying the CE and CS pins. )
RF24 radio(CE_PIN, CS_PIN); // "radio" is the identifier you will use in following methods
Bounce debouncerFire = Bounce();
Bounce debouncerReload = Bounce();

/*-----( Declare Variables )-----*/
//IR send related parameters
int timertx = 0;
int resettx = 0;
int bitstx = 0;
float elapsed = 0;
unsigned long datatx = 0;
bool spacetx = false;

// Stats
bool initial_state_sent = false;
const unsigned int MAX_AMMO = CLIPS * CLIP_SIZE;      // max ammunition
const unsigned int FIRE_RATE_DELAY = 60000 / FIRE_RATE; // get roundes per minute and convert to time delay in ms
const unsigned int RELOAD_TIMEOUT = RELOAD_TIME * 1000;
int ammo = 0;      // Current ammunition
unsigned long lastFire = 0;
unsigned long fireMessage; // unsigned long for storing fireMessage of the data which gets transmitted when the player fires.

// debouncing parameters
int value = 0;
int oldValueFire = 0;
int oldValueReload = 0;

void initGame(int isStarted) {
	if (isStarted == 1) {
		ammo = readState(EEPROM_AMMO);
	} else {
		ammo = MAX_AMMO;
		// playerId = id;
		saveState(EEPROM_AMMO, ammo);
	}

#ifdef RF_LASER_DEBUG
  Serial.println(F("initial ammo is:"));
  Serial.println(ammo);
  Serial.println(F("Fire Rate is:"));
  Serial.println(FIRE_RATE_DELAY);
#endif
  
	initPlayer(TEAM_ID, PLAYER_ID, DAMAGE);
}

void initPlayer(unsigned int teamId, unsigned short playerId, unsigned int dmg) {
  fireMessage = ((playerId & 127) << 6) | ((teamId   & 3  ) << 4) | (dmg & 15 );

#ifdef RF_LASER_DEBUG
  Serial.println(F("fire message is: "));
  Serial.println(fireMessage, BIN);
#endif
}

byte readState(int addr) {
	return EEPROM.read(addr);
}

void saveState(int addr, int val) {
	EEPROM.write(addr, val);
	//EEPROM.commit();
}

int sendMessage(int message) {
  byte recived;
  if (!radio.write(&message, 1)) {
#ifdef RF_LASER_DEBUG
    Serial.println(F("failed tx..."));
#endif
    return -1;
  } else {
#ifdef RF_LASER_DEBUG
    Serial.print(message);
    Serial.println(F(" transmitted successfully !!"));
#endif
    if(!radio.available()) {
#ifdef RF_LASER_DEBUG
      Serial.println(F("Blank Payload Received."));
#endif
      return -1;
    } else {
      while(radio.available() ){
        radio.read(&recived, 1);
#ifdef RF_LASER_DEBUG
        Serial.print(F("received ack payload is : "));
        Serial.println(recived);
#endif
      }
      return recived;
    }
  }
}

bool sendIR(unsigned long data, int nbits) {
	if (bitstx == 0) { //if IDLE then transmit
		timertx = 0; //reset timer
		TIMSK2 |= _BV(TOIE2); //activate interrupt on overflow (TOV flag, triggers at OCR2A)
		resettx = 96; //initial header pulse is 2400us long. 2400/25us ticks = 96
		spacetx = false;
		datatx = data << (32 - (nbits + 1)); //unsigned long is 32 bits. data gets   shifted so that the leftmost (MSB) will be the first of the 32.
		TCCR2A |= _BV(COM2B1); // Enable pin 3 PWM output for transmission of header
		bitstx = nbits + 1; //bits left to transmit, including header
		return true;
	} else {
		return false;
	}
}

void setup() {
#ifdef RF_LASER_DEBUG
	Serial.begin(115200);
#endif // RF_LASER_DEBUG
	
	delay(1000);

	EEPROM.begin();

	pinMode(TRIGER_PIN, INPUT_PULLUP);
	digitalWrite(TRIGER_PIN, HIGH);
	pinMode(RELOAD_PIN, INPUT_PULLUP);
	digitalWrite(RELOAD_PIN, HIGH);
	pinMode(IR_PIN, OUTPUT);
	digitalWrite(IR_PIN, LOW);
  
	// After setting up the button, setup debouncer
	debouncerFire.attach(TRIGER_PIN);
	debouncerFire.interval(5);
	// After setting up the button, setup debouncer
	debouncerReload.attach(RELOAD_PIN);
	debouncerReload.interval(5);

	//set PWM: FastPWM, OC2A as top, OC2B sets at bottom, clears on compare
  //COM2B1=1; sets PWM active on pin3, COM2B1=0 disables it
	TCCR2A = _BV(WGM21) | _BV(WGM20);
	TCCR2B = _BV(WGM22) | _BV(CS21);
	OCR2A = 49;
	OCR2B = 24;
  //OCR2A = (F_CPU/(38000*2L)-1);
  //OCR2B = OCR2A / 2;
  
	radio.begin();  // Start up the physical nRF24L01 radio
  radio.setAutoAck(1);
  radio.enableAckPayload();                     // Allow optional ack payloads
  // radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads
	//radio.setPayloadSize(1);
	radio.setChannel(RF24_CHANNEL);
	radio.setPALevel(RF24_PA_MIN);
	//radio.setPALevel(RF24_PA_MAX);  // Uncomment for more power
	//radio.setDataRate(RF24_2MBPS);
	radio.setRetries(5, 15);
	radio.openWritingPipe(pipes[GUN_TYPE]);
	radio.stopListening();
#ifdef RF_LASER_DEBUG
	Serial.println(F("Ready."));
	radio.printDetails();
#endif // RF_LASER_DEBUG
	delay(500);
}

void loop() {
	if (!initial_state_sent) {
#ifdef RF_LASER_DEBUG
    Serial.println(F("Sending init state."));
#endif
		int msg = sendMessage(INIT_MESSAGE);
#ifdef RF_LASER_DEBUG
    Serial.println(F("got init message reply:"));
    Serial.println(msg);
#endif
		if (msg >= 0 ) {
			initGame(msg);
			initial_state_sent = true;
		}
		delay(1000);
	} else {
		debouncerFire.update();
		value = debouncerFire.read();
#ifdef AUTOMATIC
		if (value == 0 && (millis() - lastFire > FIRE_RATE_DELAY)) {
#else
    if ( value == 0 && value != oldValueFire ) {
#endif
      if (((1 + MAX_AMMO - ammo) % CLIP_SIZE) == 0) {
#ifdef AUTOMATIC
        if (value != oldValueFire) {
#endif

#ifdef RF_LASER_DEBUG
				Serial.println(F("out of ammo"));
#endif
				sendMessage(EMPTY_MESSAGE);
#ifdef AUTOMATIC
        }
#endif
			} else {
				if (sendMessage(SHOT_MESSAGE) > 0) {
					sendIR(fireMessage, 14);
					ammo = ammo - 1;
					saveState(EEPROM_AMMO, ammo);
#ifdef RF_LASER_DEBUG
					Serial.println(F("Fire sent, ammo left: "));
					Serial.println(ammo);
#endif
				}
			}
			lastFire = millis();
		}		
		oldValueFire = value;

		debouncerReload.update();
		value = debouncerReload.read();
		if (value == 0 && value != oldValueReload) {
			if ((MAX_AMMO - (MAX_AMMO - ammo)) > CLIP_SIZE) {
				if (sendMessage(RELOAD_START_MESSAGE) > 0 ) {
  				delay(RELOAD_TIMEOUT);
  				if (sendMessage(RELOAD_END_MESSAGE) > 0) {
  					ammo = ammo - (ammo % CLIP_SIZE);
  					saveState(EEPROM_AMMO, ammo);
#ifdef RF_LASER_DEBUG
          Serial.println(F("reload sent, ammo left: "));
          Serial.println(ammo);
#endif
  				}
				}
			} else {
#ifdef RF_LASER_DEBUG
				Serial.println(F("out of CLIPS"));
#endif
				sendMessage(EMPTY_MESSAGE);
			}
		}
		oldValueReload = value;
  }
  delay(100);
}

ISR (TIMER2_OVF_vect, ISR_NOBLOCK) {
	//RESET_TIMER2;
	//TRANSMISSION
	if (bitstx != 0) { //if we have got something to transmit
		timertx++;
		if (timertx >= resettx) { //if reset value is reached
			timertx = 0;
			if (spacetx) { //it was a space that has just been sent thus a total "set" (bit + space) so..
				spacetx = false; //we are not going to send a space now
				bitstx = bitstx - 1;  //we got 1 less bit to send
				datatx <<= 1;  //shift it so MSB becomes 1st digit
				TCCR2A |= _BV(COM2B1);  //activate pin 3 PWM (ONE)
				if ((datatx & TOPBIT) && (bitstx != 0)) {
					resettx = 48;//48
				}
				else if (!(datatx & TOPBIT) && (bitstx != 0)) {
					resettx = 24; //24
				}
				else {
					TCCR2A &= ~(_BV(COM2B1));  //deactivate pin 3 PWM (end of transmission, no more bits to send)
					TIMSK2 &= ~(_BV(TOIE2));   //deactivate interrupts on overflow
				}
			} else {  //we sent the bit, now we have to "send" the space
				spacetx = true;  //we are sending a space
				resettx = 24; //600us/25us = 24
				TCCR2A &= ~(_BV(COM2B1));
			}
		}
	}
}
