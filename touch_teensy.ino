#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#define TOUCH0 19
#define TOUCH1 18
#define TOUCH2 17
#define TOUCH3 16
#define TOUCH4 15
#define TOUCH_THRESHOLD 800

RF24 radio(14,10);
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
byte touchReading = 0, lastTouchReading = 0;
byte rightSwipes=0, leftSwipes=0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //while(!Serial);
  delay(1500);
  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);
  
  radio.setPayloadSize(8);
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);
  radio.stopListening();
  Serial.println("Done radio init");


}

void loop() {
  // put your main code here, to run repeatedly:
  byte msg =0;

  msg = readSwipes();
  
  if (msg > 0)
  {
  
   bool ok = radio.write( &msg, sizeof(byte) );
   if(!ok)
     Serial.println("failed");
    else
      Serial.println(msg);
  }
  delay(100);
  
}

byte readSwipes()
{
  touchReading = getTouchState();
  
    if(touchReading == 0)
    {
      if (rightSwipes > 1)
      {
        Serial.println("Right swipe");
        rightSwipes = 0;
        lastTouchReading = 0;
        return 1;
      }
      if (leftSwipes > 1)
      {
        Serial.println("Left swipe");
        leftSwipes = 0;
        lastTouchReading = 0;
        return 2;
      }
    }
  
  if(gt(lastTouchReading,touchReading))
  {

    rightSwipes++;
    Serial.print("lt ");Serial.print(rightSwipes);Serial.print(" ");
    Serial.println(touchReading,BIN);
    leftSwipes=0;
  }
  
  else if(gt(touchReading,lastTouchReading))
  {
    Serial.print("gt");Serial.print(leftSwipes);Serial.print(" ");
    Serial.println(touchReading,BIN);
    leftSwipes++;
    rightSwipes=0;
  }
  
  lastTouchReading = touchReading;

  return 0;
}

byte getTouchState()
{
  byte touchState = 0;
  touchState |= getSensorState(TOUCH0);
  touchState |= getSensorState(TOUCH1)<<1;
  touchState |= getSensorState(TOUCH2)<<2; 
  touchState |= getSensorState(TOUCH3)<<3; 
  touchState |= getSensorState(TOUCH4)<<4; 
 // Serial.println(touchState, BIN);
  return touchState;
}
  
byte getSensorState(int sensor)
{
  //Serial.print(sensor);
  //Serial.print(" ");
  //Serial.println(touchRead(sensor));
  if(touchRead(sensor) > TOUCH_THRESHOLD)
    return 1;
  return 0;
}
  
bool gt(byte a, byte b)
{
  int i, j;
  for(i=128; i>0; i=i>>1)
  {
    if(a&i)
    {
      break;
    }
  }
  
  for(j=128; j>0; j=j>>1)
  {
    if(b&j)
    {
      break;
    }
  }
 
  if(i==0&&j>0)
    return true;
  if(j==0)
    return false;
  if(i>j)
    return true;
  if(i<j)
    return false;
  return gt(a-i,b-j);
}


