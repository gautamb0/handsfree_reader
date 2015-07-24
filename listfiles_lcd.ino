#include <SD.h>
#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_ST7735.h> // Hardware-specific library Leonardo
#include <Adafruit_ST7735_teensy.h> //Teensy3.1
//#include <Adafruit_HX8357_teensy.h> //himax
//#include "ILI9341_t3.h" //himax optimized
#include <Wire.h>
#include <SparkFun_APDS9960.h>

#include "RF24Soft.h"
#include "nRF24L01Soft.h"

#define TFT_CS  10  // Chip select line for TFT display
#define TFT_RST  8  // Reset line for TFT (or see below...)
#define TFT_DC   9  // Data/command line for TFT
#define SD_CS    2  // Chip select line for SD card
//Teensy 3.1
#define TFT_MOSI 11
#define TFT_SCLK 13
#define TFT_MISO 12
#define APDS9960_INT    6 // Needs to be an interrupt pin
#define RF24_CE 3
#define RF24_CS 4
#define RF24_INT 5
//MISO 12
//Use this reset pin for the shield!
//#define TFT_RST  0  // you can also connect this to the Arduino reset!

#define LINEWIDTH 21 //lcd width in pixels/6
#define LINEBUFFERSIZE LINEWIDTH * 2
#define SCREENHEIGHT 20 //lcd width in pixels/8
#define SCREENSIZE (LINEWIDTH + 1)* SCREENHEIGHT
#define NUMBUFFERS 10
#define SCREENBUFFERSIZE NUMBUFFERS * SCREENSIZE
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST); //hw spi
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST); //software spi
//Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST); //himax
//ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC); //himax opt

// Pins


// Constants

// Global Variables
SparkFun_APDS9960 apds = SparkFun_APDS9960();
int isr_flag = 0, radio_isr_flag = 0;

RF24 radio(RF24_CE,RF24_CS);
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };


File root;
File dataFile;
char lineBuffer[LINEBUFFERSIZE] ={
  '\0'};
char screenBuffer[SCREENBUFFERSIZE] ={
  '\0'};
int screenBufferPos = 0, lastScreen = -1, currScreen = -1, startpos = LINEBUFFERSIZE;
// change this to match your SD shield or module;
//     Arduino Ethernet shield: pin 4
//     Adafruit SD shields and modules: pin 10
//     Sparkfun SD shield: pin 8
const int chipSelect = 2;
/*const int rightButtonPin = 4;     // the number of the pushbutton pin
const int leftButtonPin = 3;*/ 
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 25;    // the debounce time; increase if the output flickers

void setup()
{


  //int i =0;
  //for(i ; i < 50000; i++)
  //Serial.println(i);
  // Open serial communications and wait for port to open:
 Serial.begin(9600);
 // while (!Serial) {
   // ; // wait for serial port to connect. Needed for Leonardo only
 // }
  delay(2500);
  Serial.print("Initializing SD card...");



  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on Arduino Uno boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
  pinMode(SS, OUTPUT);
 /* pinMode(rightButtonPin, INPUT);
  pinMode(leftButtonPin, INPUT);*/
  pinMode(APDS9960_INT, INPUT);
  pinMode(RF24_CS,OUTPUT);

  SD.begin(chipSelect);
  attachInterrupt(APDS9960_INT, gestureInterruptRoutine, FALLING);
  
  apds.init();
  apds.enableGestureSensor(true);
 

  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);
  
  radio.setPayloadSize(8);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();
  
  pinMode(RF24_INT, INPUT);
  attachInterrupt(RF24_INT, radioInterruptRoutine, FALLING);
  //Serial.println("Done radio init");
 
/*  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
  }

  Serial.println("initialization done.");
  if ( apds.init() ) {
    Serial.println(F("APDS-9960 initialization complete"));
  } else {
    Serial.println(F("Something went wrong during APDS-9960 init!"));
  }
  
  // Start running the APDS-9960 gesture sensor engine
  if ( apds.enableGestureSensor(true) ) {
    Serial.println(F("Gesture sensor is now running"));
  } else {
    Serial.println(F("Something went wrong during gesture sensor init!"));
  }
*/
  root = SD.open("/");
  // large block of text

  tft.initR(); 
  //tft.begin(HX8357D);
  //tft.begin();
  //tft.setRotation(3);

  //tft.fillScreen(HX8357_BLACK);
  //tft.setTextColor(HX8357_WHITE);
  //   tft.fillScreen(ILI9341_BLACK);
  //tft.setTextColor(ILI9341_WHITE);

  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);

  tft.setTextWrap(true);

  tft.setCursor(0, 0);


  printDirectory(root, 0);

 // Serial.println("done!");
}

byte handleGesture() {
    if ( apds.isGestureAvailable() ) {
    switch ( apds.readGesture() ) {
      case DIR_UP:
        //Serial.println("UP");
        break;
      case DIR_DOWN:
        //Serial.println("DOWN");
        break;
      case DIR_LEFT:

        //Serial.println("LEFT");
        return B10;
        break;
      case DIR_RIGHT:
        //Serial.println("RIGHT");
        return B01;
        break;
      case DIR_NEAR:
        //Serial.println("NEAR");
        break;
      case DIR_FAR:
        //Serial.println("FAR");
        break;
      default:
        break;
        //Serial.println("NONE");
    }
  }
  return 0x0;
}

void loop()
{
 
  //delay(1200);

/*  boolean left = digitalRead(leftButtonPin);
  boolean right = digitalRead(rightButtonPin);
  int reading = right| (left<<1);
  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  } 
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    buttonState = reading;
  }

  if ((buttonState&1)&!(lastButtonState&1)) {
    advanceScreen();
  }
  else if((buttonState>>1)&!(lastButtonState>>1)) {
    rewindScreen();
  }
  lastButtonState = reading;
  advanceScreen();
  delay(5000);*/
}

void gestureInterruptRoutine() {
 byte gestureReading = 0;
 gestureReading = handleGesture();
    
  if(gestureReading == 1)
  {    
    advanceScreen();
    radio.stopListening(); 
    bool ok = radio.write( &gestureReading, sizeof(byte) );
    if(!ok)
     Serial.println("failed");
    else
      Serial.println(gestureReading);
    radio.startListening();
    return;
  }
  if(gestureReading == 2)
  {
    rewindScreen();
    radio.stopListening();
    bool ok = radio.write( &gestureReading, sizeof(byte) );
    if(!ok)
     Serial.println("failed");
    else
      Serial.println(gestureReading);
    radio.startListening();
    return;
  }
  
}

void radioInterruptRoutine() {
  byte payload = 0;
  Serial.println("Interrupt");
    if(radio.available())
    {

      bool done = false;
    
      while(!done)
      {
        done = radio.read(&payload, sizeof(byte));
       Serial.print("Got payload ");
      Serial.println(payload);  
      }
      Serial.println("Payload done");
    }
 
  if(payload == 1)
  {
    advanceScreen();
    return;
  }
  if(payload == 2)
  {
    rewindScreen();
    return;
  }
 
}

void advanceScreen()
{

  if(currScreen == NUMBUFFERS-1) //in all other cases, the screen to display is already buffered (buffer is circular)
  {
    currScreen = 0;
  }
  else
  {
    currScreen++;
  }
  if((currScreen == lastScreen + 1)||(currScreen == 0 && lastScreen == NUMBUFFERS-1))  //only need to fill the buffer in the case when the screen needing to be displayed is one ahead of the last filled screen (the typical case)
  { 
    fill_buffer(currScreen);

  }
  print_page(currScreen);


}

void rewindScreen()
{
  if((currScreen == lastScreen + 1)||(currScreen == 0 && lastScreen == NUMBUFFERS-1)) //beginning of the circular buffer, can't go back farther
  {
    return;
  }
  if(currScreen == 0)
  {
    currScreen = NUMBUFFERS-1;
  }
  else
  {
    currScreen--;
  }
  print_page(currScreen);
}


void printDirectory(File dir, int numTabs) {
  // Begin at the start of the directory
  dir.rewindDirectory();

  while(true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      //Serial.println("**nomorefiles**");
      break;
    }
    //    for (uint8_t i=0; i<numTabs; i++) {
    //      Serial.print('\t');   // we'll have a nice indentation
    //    }
    // Print the 8.3 name
    //Serial.print(entry.name());
    //screenBufferPrint(entry.name());
    if(strstr(entry.name(),".TXT")!=NULL)
    {

      //screenBufferPrintln(entry.name());
      //screenBufferPrintln("\n");
      dump_file(entry.name());
    }
    // Recurse for directories, otherwise print the file size
    if (entry.isDirectory()) {
      //Serial.println("/");
      printDirectory(entry, numTabs+1);
    } 
    else {
      // files have sizes, directories do not
      //Serial.print("\t\t");
      //Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void dump_file(char* fileName)
{
  dataFile = SD.open(fileName);


}

void fill_buffer(int screenNum) //which screen in the buffer 0-4
{

  screenBufferPos = screenNum * SCREENSIZE;
  int i = 0;

  for(i = 0; i<SCREENHEIGHT;i++)
  {
    print_line();
   // Serial.print("Line done ");
    //Serial.println(i);
  }
  lastScreen = screenNum; //mark this screen in the buffer as filled
}

void print_page(int screenNum)
{
  tft.setCursor(0, 0);  
  //tft.fillScreen(ILI9341_WHITE);
  //tft.setTextColor(ILI9341_BLACK);

  //tft.fillScreen(HX8357_WHITE);
  //tft.setTextColor(HX8357_BLACK);

  tft.fillScreen(ST7735_WHITE);
  tft.setTextColor(ST7735_BLACK);
  int offset = screenNum * SCREENSIZE;
  int i; 


  for(i = 0 + offset; i < SCREENSIZE + offset; i ++)
  {
    tft.print(screenBuffer[i]);
     /*Serial.print(i);
     Serial.print(" : ");
     if(screenBuffer[i]=='\n')
     Serial.println("NL");
     else if(screenBuffer[i]=='\0')
     Serial.println("NULL");
     else if(screenBuffer[i]==' ')
     Serial.println("SPACE");
     else
     Serial.println(screenBuffer[i]);*/
    //screenBuffer[i] = '\0';
  }
  //Serial.println("\n===========================================================");
}

void print_line()
{
  int i=0,j=0,k=0,cursorpos=0;
  char temp;
  for(i=startpos;i<LINEBUFFERSIZE;i++)
  {
    if(lineBuffer[i]=='\0')//end of buffer
      break;

    screenBufferPrint(lineBuffer[i]);
    cursorpos++; //keep track of physical horizontal position of cursor
    if(lineBuffer[i]=='\n')
    {
      lineBuffer[i]='\0';//replace the buffer with a null character to "erase" it
      startpos=LINEBUFFERSIZE;
      return;
    }
    if(lineBuffer[i]==32)
    {
      lineBuffer[i]='\0';
      break;
    }
    lineBuffer[i]='\0';
  }  
  if(dataFile.available()){
    //screenBufferPrint("Starting:\n");
    for(i=0; i < LINEBUFFERSIZE; i++)  //buffer is double the line width
    {
      temp = (char)dataFile.read();
      if(temp==13)
      {
        temp = (char)dataFile.read();
      }
      /*if(temp==10)
      {
        temp = 32;
      }*/
      lineBuffer[i] = temp;
      if(lineBuffer[i]=='\n')
        break;
     /* Serial.print(i);
      Serial.print(", ");
      Serial.print(cursorpos);
      Serial.print(": ");
      int asInt = lineBuffer[i];
      Serial.print(asInt);
      Serial.print(" ");
      if(lineBuffer[i]=='\n')
        Serial.println("NL");
      else if(lineBuffer[i]=='\0')
        Serial.println("NULL");
      else if(lineBuffer[i]==' ')
        Serial.println("SPACE");
      else
        Serial.println(lineBuffer[i]);*/
      if(i>=LINEWIDTH-cursorpos&&lineBuffer[i]==32) //first space after the end of this line
        break;
    }

    if(i<LINEWIDTH-cursorpos&&lineBuffer[i]=='\n')
      j = i + 1;
    else if(i==LINEWIDTH-cursorpos&&(lineBuffer[i]==32||lineBuffer[i]=='\n')) //degenerate case where the first character of the next line is a space, same as the space following the last word on this line
    { 
      j = i;
    }
    else
    {
      for(j = i - 1; j >= 0; j--) //find the space after the last word of this line
      {
        if(lineBuffer[j] == 32)
          break;
      }
    }

    for(k = 0; k < j; k++)
    {
      screenBufferPrint(lineBuffer[k]);
      if(lineBuffer[k]=='\n')
      {
        lineBuffer[k]='\0';
        startpos=LINEBUFFERSIZE;
        return;
      }
      lineBuffer[k]='\0';
    
    }
    lineBuffer[j]='\0';
    if(j!=i)
    {
     /* int numSpaces = LINEWIDTH-cursorpos - j, count;
      for(count = 0; count < numSpaces; count++)
      {
        screenBufferPrint(32);
      }*/
      screenBufferPrint('\n');
      startpos=j+1;
    }
    else
      startpos=LINEBUFFERSIZE; //ignore the buffer, as it does not contain a part of the next line     

  }
}

void screenBufferPrint(char c)
{
  screenBuffer[screenBufferPos] = c; 
  /* Serial.print(screenBufferPos);
   Serial.print(" : ");
   if(  screenBuffer[screenBufferPos]=='\n')
   Serial.println("NL");
   else if(  screenBuffer[screenBufferPos]=='\0')
   Serial.println("NULL");
   else if(  screenBuffer[screenBufferPos]==' ')
   Serial.println("SPACE");
   else
   Serial.println(  screenBuffer[screenBufferPos]);
   */
  screenBufferPos++;


}



