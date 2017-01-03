#ifndef RfLaserTag_h
  #define RfLaserTag_h

  #include <Arduino.h>

  #define RF_LASER_DEBUG //comment to stop debuging to serial
  #define PLAYER_ID 1
  #define TEAM_ID 1

  //radio staff
  const uint64_t pipes[3] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0E2LL, 0xF0F0F0F0E3LL };
  #define RF24_CHANNEL 108 // Above most Wifi Channels
  
  //messages from gun 
  #define INIT_MESSAGE 1
  #define SHOT_MESSAGE 2
  #define EMPTY_MESSAGE 3
  #define RELOAD_START_MESSAGE 4
  #define RELOAD_END_MESSAGE 5
  
  // weapons types
  #define PRIMARY_GUN 1
  #define SECONDARY_GUN 2

#endif;