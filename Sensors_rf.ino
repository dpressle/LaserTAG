/*-----( Import needed libraries )-----*/
#include <SPI.h>
#include <SD.h>
#include <pcmConfig.h>
#include <pcmRF.h>
#include <TMRpcm.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <IRremote.h>
/*-----( Declare Constants and Pin Numbers )-----*/
#define RECV_PIN 5
#define CE_PIN 7
#define CS_PIN 8
#define SPEAKER_PIN 9
#define SD_CS_PIN 4
#define HELLO 0
#define SHOOT 1
#define NO_AMMO 2
#define RELOAD 3
#define HELLO_SOUND "0.wav"
#define SHOOT_SOUND "01.wav"
#define NO_AMMO_SOUND "02.wav"
#define CLIP_SOUND "03.wav"
#define RELOAD_SOUND "04.wav"
#define MISS_SOUND "05.wav"
#define DEAD_SOUND "06.wav"
#define PIPE 0xE8E8F0F0E1LL

/*-----( Declare objects )-----*/
// (Create an instance of a radio, specifying the CE and CS pins. )
RF24 myRadio (CE_PIN, CS_PIN); // "myRadio" is the identifier you will use in following methods
IRrecv irrecv(RECV_PIN);
TMRpcm tmrpcm;   // create an object for use in this sketch
/*-----( Declare Variables )-----*/
decode_results results;
int dead = 0;
int msg[1];
int dataReceived;  // Data that will be received from the transmitter
int ack = 0;
byte addresses[][6] = {"1Node"}; // Create address for 1 pipe.
const uint64_t pipe = PIPE;

void setup()
{
  Serial.begin(115200);

  tmrpcm.speakerPin = SPEAKER_PIN;
  SD.begin(SD_CS_PIN);
  tmrpcm.volume(1);

  irrecv.enableIRIn(); // Start the receiver

  //const uint64_t pipe = 0xE8E8F0F0E1LL;
  myRadio.begin();
  myRadio.setAutoAck(1);
  myRadio.setChannel(108);  // Above most Wifi Channels
  myRadio.setPALevel(RF24_PA_MIN);
  //myRadio.setPALevel(RF24_PA_MAX);  // Uncomment for more power
  //myRadio.setDataRate(RF24_2MBPS);
  myRadio.setRetries(5, 15);
  myRadio.openReadingPipe(1, pipe);
  myRadio.startListening();

  Serial.println("Ready");
}

void loop() {
  if (!dead)
  {
    if (irrecv.decode(&results)) {
      Serial.println(results.value, BIN);
      irrecv.resume(); // Receive the next value
      if (results.bits == 14) {
        dead++;
      }
    }
    if (myRadio.available()) {
      while (myRadio.available())  // While there is data ready
      {
        myRadio.read(&dataReceived, sizeof(dataReceived)); // Get the data payload (You must have defined that already!)
      }
      //myRadio.writeAckPayload( 1, &ack, sizeof(ack) );
      Serial.println(dataReceived);
      switch (dataReceived) {
        case HELLO:
          Serial.println("got HELLO message");
          tmrpcm.play(HELLO_SOUND);
          break;
        case SHOOT:
          Serial.println("got SHOOT message    ");
          tmrpcm.play(SHOOT_SOUND);
          break;
        case NO_AMMO:
          Serial.println("got NO_AMMO message");
          tmrpcm.play(NO_AMMO_SOUND);
          break;
        case RELOAD:
          Serial.println("got RELOAD message");
          tmrpcm.play(CLIP_SOUND);
          delay(2000);
          tmrpcm.play(RELOAD_SOUND);
          break;
        default:
          Serial.println("Message not recognized");
          break;
      }
    } else {
      //Serial.println("No radio available");
    }
  }
  delay(100);
}

