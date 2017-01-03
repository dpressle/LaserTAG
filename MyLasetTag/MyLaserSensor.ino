// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24

#define MY_RF24_PA_LEVEL RF24_PA_LOW

#define MY_NODE_ID 5

#include <MySensors.h>
#include <SPI.h>
#include <MyLaserTag.h>
#include <IRremote.h>

#define IR_PIN 3
#define CHILD_ID_SENSOR_HEAD 0   // sensor Id of the sensor child

const bool IS_ACK = false; //is to acknowlage
decode_results results;

// Stats
//static bool game_started = false;//
static bool dead = true;

IRrecv irrecv(IR_PIN);

MyMessage sensorMessage(CHILD_ID_SENSOR_HEAD, V_SENSOR_HEAD);
// MyMessage bodySensorMessage(CHILD_ID_SENSOR_BODY, V_SENSOR_BODY);

void receive(const MyMessage &message) {
  switch (message.type) {
	case V_COMMAND:
	  int messagePayload = message.getInt();
	  switch (messagePayload) {
	    case START_GAME_MESSAGE:
		  dead = false;
		  //game_started = true;
		  //saveState(CHILD_ID_SENSOR, 0);
		  break;
	    case END_GAME_MESSAGE:
		  dead = true;
		  //game_started = false;
		  //saveState(CHILD_ID_SENSOR, 1);
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
	present(CHILD_ID_SENSOR_HEAD, S_CUSTOM, PRESENT_MESSAGE, IS_ACK);
}

void before() {
  //  attachInterrupt(digitalPinToInterrupt(BUTTON_DOWN_PIN), downButtonPress, FALLING);
}

void setup(void) {
  irrecv.enableIRIn(); // Start the receiver
  sensorMessage.setDestination(0);
  //dead = loadState(CHILD_ID_SENSOR) == 0 ? false : true ;
}

void loop(void) {
  if (!init_state_send) {
    send(sensorMessage.set(INIT_MESSAGE));
	init_state_send = true;
  } else if (!dead) {
    if (irrecv.decode(&results)) {
      if (results.bits == 14) {
        send(sensorMessage.set(HIT_MESSAGE));
      } else {
	    send(sensorMessage.set(MISS_MESSAGE));
	  }
      irrecv.resume(); // Receive the next value
    }
  }
}

