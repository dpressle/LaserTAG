#ifndef MyLaserTag_h
  #define MyLaserTag_h

  #include <Arduino.h>
  #include <MySensors.h>
  
  #define GATEWAY_NODE 0
  #define PRIMARY_GUN_NODE 1
  #define SECONDARY_GUN_NODE 2
  #define HEAD_SENSOR_NODE 5
  #define BODY_SENSOR_NODE 6
  #define GUN_SENSOR_NODE 7
  
  //gateway to nodes message types
  #define V_COMMAND V_VAR1
  #define V_SYSTEM V_VAR2
  #define V_STATE V_VAR3
  #define V_ID V_VAR4
  
  //nodes to gateway message types
  #define V_GUN V_VAR1
  #define V_SENSOR_GUN V_VAR2
  #define V_SENSOR_HEAD V_VAR3
  #define V_SENSOR_BODY V_VAR4
  
  //messages
  #define INIT_MESSAGE 0
  #define READY_MESSAGE 1
  #define START_GAME_MESSAGE 2
  #define END_GAME_MESSAGE 3
  #define FIRE_MESSAGE 4
  #define EMPTY_MESSAGE 5
  #define RELOAD_START_MESSAGE 6
  #define RELOAD_END_MESSAGE 7
  #define HIT_MESSAGE 8
  #define MISS_MESSAGE 9
  #define ID_MESSAGE 10
  
  // weapons types
  #define RIFLE 1
#endif;