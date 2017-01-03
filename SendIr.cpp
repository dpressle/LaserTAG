
#define IR_PWM_FREQUENCY 38 // What frequency is the IR
#define TX_IO_Pin 3    // Pin to send data on

#if IR_PWM_FREQUENCY == 38
  #define PWM_DELAY 13
  #define IR_CLOCK_RATE 38000
#endif

void MT5_TX_init(){
  // toggle on compare, clk/1
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS20);
  OCR2A = (F_CPU/(IR_CLOCK_RATE*2L)-1);
  OCR2B = OCR2A; // / 2;
  pinMode(TX_IO_Pin, OUTPUT);
  digitalWrite(TX_IO_Pin, LOW); 
}

void MT5_TX(long uSecs){
    TCCR2A |= _BV(COM2B0);
    delayMicroseconds(uSecs); //PWM_DELAY - 3);
    TCCR2A &= ~(_BV(COM2B0));
}

void MT5_TX_header(){
  MT5_TX(2400);
  delayMicroseconds(600);
}

void MT5_TX_logic1(){
  MT5_TX(1200);
  delayMicroseconds(600);
}

void MT5_TX_logic0(){
  MT5_TX(600);
  delayMicroseconds(600);
}


void MT5_TX_byte(unsigned char dataByte){
  for(signed char byteBit=7; byteBit>=0; byteBit--){
    if((dataByte >> byteBit) & 0x01 == 1){
      MT5_TX_logic1();
    }else{
      MT5_TX_logic0();
    }
  } 
}
void MT5_TX_shot(unsigned char teamID, unsigned char playerID, unsigned char damage){
  MT5_TX_header();
  MT5_TX_byte(playerID  & 0x7F);
  MT5_TX_byte( ( (teamID & 0x03) << 6 ) + ((damage & 0x0F) << 2) );
}