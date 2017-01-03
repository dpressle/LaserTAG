// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24

#define MY_RF24_PA_LEVEL RF24_PA_LOW

#define MY_NODE_ID 1

#include <Bounce2.h>
#include <MySensors.h>
#include <SPI.h>
#include <MyLaserTag.h>
#include <MyRifle.h>

#define BUTTON_SHOOT_PIN  2  // Arduino Digital I/O pin number for up button
#define BUTTON_RELOAD_PIN  4  // Arduino Digital I/O pin number for down button
#define IR_PIN 3

#define CHILD_ID_GUN 0   // sensor Id of the sensor child
#define PLAYER_ID 0   // sensor Id of the sensor child
#define AMMO 1

const bool IS_ACK = false; //is to acknowlage

//IR send related parameters
int timertx = 0; 
int resettx = 0; 
int bitstx = 0; 
float elapsed = 0; 
unsigned long datatx = 0; 
boolean spacetx = false;

// Stats
static bool initial_state_sent = false;//
static bool dead = true;
int maxAmmo = CLIPS * CLIP_SIZE;      // max ammunition
int ammo = 0;      // Current ammunition
unsigned long lastFire = 0;
int fireMessage[16];                        // String for storing fireMessage of the data which gets transmitted when the player fires.
noPlayerId = true;
bool gameStarted = false;

// debouncing parameters
int value = 0;
int oldValueShoot = 0;
int oldValueReload = 0;

Bounce debouncerShoot = Bounce();
Bounce debouncerReload = Bounce();

MyMessage gunMessage(CHILD_ID_GUN, V_GUN);
// MyMessage sysMessage(CHILD_ID_SYS, V_INIT);

bool sendMessage(int message){
  return send(gunMessage.set(message);
}

void receive(const MyMessage &message) {
  int sender = message.sender;
  int type = message.type;
  int sensor = message.sensor;
  int destination = message.destination;
  int messagePayload = message.getInt();
  
  switch (type) {
    case V_STATE:
	  switch (messagePayload) {
	    case GAME_ON:
		  ammo = loadState(AMMO);
		  dead = false
		  break;
	    case GAME_OFF:
		  initGame();
		  sendMessage(READY_MESSAGE);
		  break;
		default:
	      break;
	  }
    case V_ID:
	  initPlayer(messagePayload);
	  break;
	case V_COMMAND:
	  switch (messagePayload) {
	    case START_GAME_MESSAGE:
		  dead = false;
		  break;
	    case END_GAME_MESSAGE:
		  dead = true;
		  break;
		default:
	      break;
	  }
	default:
	  break;
  }
  return;
}

void presentation() {
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo(SKETCH_NAME, SKETCH_VER);
	// Register all sensors to gw (they will be created as child devices)
	present(CHILD_ID_GUN, S_CUSTOM, PRESENT_MESSAGE, IS_ACK);
}

void before() {
  // Setup the button
  pinMode(BUTTON_SHOOT_PIN_PIN, INPUT_PULLUP);
  //Activate internal pull-up
  digitalWrite(BUTTON_SHOOT_PIN_PIN, HIGH);
  //attachInterrupt(digitalPinToInterrupt(BUTTON_UP_PIN), upButtonPress, FALLING);
  
  pinMode(BUTTON_RELOAD_PIN, INPUT_PULLUP);
  // Activate internal pull-up
  digitalWrite(BUTTON_RELOAD_PIN, HIGH);
  //  attachInterrupt(digitalPinToInterrupt(BUTTON_DOWN_PIN), downButtonPress, FALLING);
  pinMode(IR_PIN, OUTPUT);
  digitalWrite(IR_PIN, LOW);

  // After setting up the button, setup debouncer
  debouncerShoot.attach(BUTTON_FIRE_PIN_PIN);
  debouncerShoot.interval(5);
  // After setting up the button, setup debouncer
  debouncerReload.attach(BUTTON_RELOAD_PIN);
  debouncerReload.interval(5);
  
  //set PWM: FastPWM, OC2A as top, OC2B sets at bottom, clears on compare
  //COM2B1=1 sets PWM active on pin3, COM2B1=0 disables it
  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS21);
  OCR2A = 49;
  OCR2B = 24;
}

void setup(void) {
  initial_state_sent = false;
  gunMessage.setDestination(GATEWAY_NODE);
  
  int playerId = loadState(PLAYER_ID);
  if (playerId == 0xffff) { 
    sendMessage(ID_MESSAGE);
  } else {
    initPlayer(playerId);
  }
}

void loop(void) {
  if (!initial_state_sent) {
    // while (!sendMessage(INIT_MESSAGE) {
	  // wait(1000);
	// }
    // initial_state_sent = true;
	Serial.println("Sending initial value");
    sendMessage(INIT_MESSAGE);
    Serial.println("Requesting initial value from controller");
    //request(CHILD_ID, V_STATUS);
    wait(2000, C_SET, V_GUN);
  } else if (!dead) {
	  debouncerFire.update();
	  value = debouncerFire.read();
	  if (value == 0 && value != oldValueFire && (millis() - lastFire > FIRE_RATE) ) {
		if (((maxAmmo - ammo) % CLIP_SIZE) == 0) {
		  #ifdef MY_DEBUG
		    Serial.println("out of ammo");
		  #endif
		  sendMessage(EMPTY_MESSAGE);
		} else {
		  lastFire = millis();
		  sendIR(fireMessage, 14);
		  sendMessage(FIRE_MESSAGE);
		  ammo = ammo - 1;
		  saveState(AMMO, ammo);
		  Serial.println(ammo);
		}
	  }
	  oldValueFire = value;

	  debouncerReload.update();
	  value = debouncerReload.read();
	  if (value == 0 && value != oldValueReload) {
	   if ((maxAmmo - (maxAmmo - ammo)) > CLIP_SIZE) {
		  sendMessage(RELOAD_START_MESSAGE);
		  wait(RELOAD_TIMEOUT);
		  sendMessage(RELOAD_END_MESSAGE);
		  ammo = ammo - (ammo % CLIP_SIZE);
		  saveState(CHILD_ID, ammo);
		} else {
		  #ifdef MY_DEBUG
		    Serial.println("out of CLIPS");
		  #endif
		  sendMessage(EMPTY_MESSAGE);
		}
	  }
	  oldValueReload = value;
  }
}

void initGame() {
  ammo = maxAmmo;
}

void initPlayer(playerId){
  //allways 0 for fire command
  fireMessage[0] = 0;
  //player ID
  fireMessage[1] = playerId >> 12 & B1;
  fireMessage[2] = playerId >> 11 & B1;
  fireMessage[3] = playerId >> 10 & B1;
  fireMessage[4] = playerId >> 9 & B1;
  fireMessage[5] = playerId >> 8 & B1;
  fireMessage[6] = playerId >> 7 & B1;
  fireMessage[7] = playerId >> 6 & B1;
  //team ID
  fireMessage[8] = 0;
  fireMessage[9] = 0;
  // weapon damage
  fireMessage[10] = DAMAGE >> 3 & B1;
  fireMessage[11] = DAMAGE >> 2 & B1;
  fireMessage[12] = DAMAGE >> 1 & B1;
  fireMessage[13] = DAMAGE >> 0 & B1;
  saveState(PLAYER_ID, playerId);
}

boolean sendIR(unsigned long data, int nbits) {
  if (bitstx == 0) { //if IDLE then transmit
    timertx = 0; //reset timer
    TIMSK2 |= _BV(TOIE2); //activate interrupt on overflow (TOV flag, triggers at OCR2A)
    resettx = 96; //initial header pulse is 2400us long. 2400/25us ticks = 96
    spacetx = false;
    datatx = data << (32 - (nbits + 1)); //unsigned long is 32 bits. data gets   shifted so that the leftmost (MSB) will be the first of the 32.
    TCCR2A |= _BV(COM2B1); // Enable pin 3 PWM output for transmission of header
    bitstx = nbits + 1; //bits left to transmit, including header
    return true;
  }
  else {
    return false;
  }
}

ISR(TIMER2_OVF_vect, ISR_NOBLOCK) {
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
          resettx = 48;
        }
        else if (!(datatx & TOPBIT) && (bitstx != 0)) {
          resettx = 24;
        }
        else {
          TCCR2A &= ~(_BV(COM2B1));  //deactivate pin 3 PWM (end of transmission, no more bits to send)
          TIMSK2 &= ~(_BV(TOIE2));   //deactivate interrupts on overflow
        }
      }
      else {  //we sent the bit, now we have to "send" the space
        spacetx = true;  //we are sending a space
        resettx = 24; //600us/25us = 24
        TCCR2A &= ~(_BV(COM2B1));
      }
    }
  }
}
