/*-----( Import needed libraries )-----*/
#include <SPI.h>
#include "RF24.h"

/*-----( Declare Constants and Pin Numbers )-----*/
#define TOPBIT 0x80000000
#define TRIGER_PIN 2
#define IR_PIN 3
#define RELOAD_PIN 4
#define AUTO_PIN 5
#define CE_PIN 9
#define CS_PIN 10
#define HELLO 0
#define SHOOT 1
#define NO_AMMO 2
#define RELOAD 3
#define PIPE 0xE8E8F0F0E1LL
/*-----( Declare objects )-----*/
// (Create an instance of a radio, specifying the CE and CS pins. )
RF24 myRadio (CE_PIN, CS_PIN); // "myRadio" is the identifier you will use in following methods

/*-----( Declare Variables )-----*/
const uint64_t pipe = PIPE;
//int msg[1];
//byte addresses[][6] = {"1Node"}; // Create address for 1 pipe.
//int dataTransmitted;  // Data that will be Transmitted from the transmitter

//IR send related parameters
int timertx = 0; int resettx = 0; int bitstx = 0; float elapsed = 0; unsigned long datatx = 0; boolean spacetx = false;

int FIRE                   = 0;      // 0 = don't fire, 1 = Primary Fire
int reload_flag                 = 0;      // 0 = don't reload, 1 = reload
int TR                     = 0;      // Shoot Trigger Reading
int LTR                    = 0;      // Last Shoot Trigger Reading
int RR                     = 0;      // Reload trigger Reading
int RTR                    = 0;      // Last Reload Trigger Reading
int automatic              = 0;
// Stats
int ammo                   = 0;      // Current ammunition
int maxAmmo                = 30;     // max ammunition
int mags                   = 5;      // Current mags

void setup() {
  // Use the serial Monitor (Symbol on far right). Set speed to 115200 (Bottom Right)
  Serial.begin(115200);
  delay(1000);
  pinMode(TRIGER_PIN, INPUT_PULLUP);
  digitalWrite(TRIGER_PIN, HIGH);
  pinMode(RELOAD_PIN, INPUT_PULLUP);
  digitalWrite(RELOAD_PIN, HIGH);
  pinMode(AUTO_PIN, INPUT_PULLUP);
  digitalWrite(AUTO_PIN, HIGH);
  pinMode(IR_PIN, OUTPUT);
  digitalWrite(IR_PIN, LOW);

  //set PWM: FastPWM, OC2A as top, OC2B sets at bottom, clears on compare
  //COM2B1=1 sets PWM active on pin3, COM2B1=0 disables it
  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS21);
  OCR2A = 49;
  OCR2B = 24;

  myRadio.begin();  // Start up the physical nRF24L01 Radio
  //myRadio.setAutoAck(true);
  //myRadio.enableAckPayload();                     // Allow optional ack payloads
  //myRadio.enableDynamicPayloads();                // Ack payloads are dynamic payloads
  myRadio.setChannel(108);  // Above most Wifi Channels
  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  myRadio.setPALevel(RF24_PA_MIN);
  //  myRadio.setPALevel(RF24_PA_MAX);  // Uncomment for more power
  //myRadio.setDataRate(RF24_2MBPS);
  myRadio.setRetries(5, 15);
  myRadio.openWritingPipe(pipe); // Use the first entry in array 'addresses' (Only 1 right now)
  myRadio.stopListening();
  //first connect, we need the reciver to answer our call
  // while (!sendRf(HELLO)) {
  //   Serial.println("NO answer from reciever yet, sleeping for 500...");
  //   delay(500);
  // }

  ammo = maxAmmo;
  Serial.println("Ready.");
  delay(500);
}

void loop() {
  triggers();
  if (FIRE != 0) shoot();
  if (reload_flag != 0) reload();
}

void triggers() {
  LTR = TR;       // Records previous state. Primary fire
  RTR = RR;     // Records previous state. Secondary fire
  TR = digitalRead(TRIGER_PIN);      // Looks up current trigger button state
  RR = digitalRead(RELOAD_PIN);    // Looks up current trigger button state
  automatic = digitalRead(AUTO_PIN);
  // Code looks for changes in trigger state to give it a semi automatic shooting behaviour
  if (TR != LTR && TR == LOW) {
    Serial.println("FIRE...");
    FIRE = 1;
  }
  if (TR == LOW && automatic == 1) {
    Serial.println("FIRE AUTO...");
    FIRE = 1;
  }
  if (RR != RTR && RR == LOW) {
    Serial.println("RELOAD...");
    reload_flag = 1;
  }
}

void shoot() {
  if (ammo < 1 || mags < 1) {
    Serial.println("out of ammo");
    sendRf(NO_AMMO);
  } else {
    if (sendRf(SHOOT)) {
      sendIR(0b00000001000000, 14);
      ammo = ammo - 1;
      Serial.println(ammo);
    }
  }
  FIRE = 0;
}

void reload() {
  if (mags < 1) {
    Serial.println("Out of mags");
    sendRf(NO_AMMO);
  } else if (sendRf(RELOAD)) {
    ammo = maxAmmo;
    mags = mags - 1;
    Serial.println("reload...");
  }
  reload_flag = 0;
}

boolean sendRf(int message) {
  //dataTransmitted = message;
  if (myRadio.write(&message, sizeof(message)))
  {
    Serial.print( message );
    Serial.println("  transmitted successfully !!");
    return true;
  }
  else {
    Serial.println("failed tx...");
    return false;
  }
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
