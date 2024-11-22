#include "p103_board.h"

#include "clocks.h"

P103_board::P103_board():
  ledPin(PC,12),
  led(ledPin),
  buttonPin(PA,0),
  button(buttonPin){
  /*empty*/
}

void P103_board::toggleLed(){
  led.toggle();//low level access has optimal way of doing this.
}
