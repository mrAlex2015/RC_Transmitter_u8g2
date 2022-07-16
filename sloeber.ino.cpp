#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2022-07-15 23:08:31

#include "Arduino.h"
#include "Arduino.h"
#include <SPI.h>
#include <Wire.h>
#include <RF24.h>
#include <printf.h>
#include <EEPROMex.h>
#include <statusLED.h>
#include <U8g2lib.h>
#include <avr/eeprom.h>
extern const char codeCompileVersion[];
extern const char codeWelcome[];
extern const char codeNullString[];
extern const char codeCompileDetails[];
#define   __I   volatile const
#define   __O   volatile
#define   __IO   volatile
extern const char eepromstrDate[];
extern const char eepromstrTime[];
extern const float codeVersion;
extern struct TypeEEPROMStruct_dev MyStruct;
#define BOOL_EEPROMADR(x) (unsigned int) ((bool *)&x)
#define UCHAR_EEPROMADR(x) (unsigned int) ((unsigned char *)&x)
extern volatile byte transmissionMode;
extern volatile byte operationMode;
extern int vehicleNumber;
extern byte NRFchannel;
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const uint64_t pipeOut[];
extern const int maxVehicleNumber;
extern RF24 radio;
extern boolean transmissionState;
extern int pfChannel;
extern const int pfMaxAddress;
extern boolean batteryOkTx;
#define BATTERY_DETECT_PIN A7
extern float txVcc;
extern float txBatt;
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern const byte eeprom_ChannelVulue[];
extern byte channelValue[];
#define JOYSTICK_1 A1
#define JOYSTICK_2 A0
#define JOYSTICK_3 A3
#define JOYSTICK_4 A2
#define JOYSTICK_BUTTON_LEFT 4
#define JOYSTICK_BUTTON_RIGHT 2
extern byte leftJoystickButtonState;
extern byte rightJoystickButtonState;
#define BUTTON_LEFT   1
#define BUTTON_RIGHT  10
#define BUTTON_SEL    0
#define BUTTON_BACK   9
extern byte leftButtonState;
extern byte rightButtonState;
extern byte selButtonState;
extern byte backButtonState;
#define DRE(signal, state) (state=(((state<<1)|(signal&1))&15))==7
extern statusLED greenLED;
extern statusLED redLED;
#define HW_I2C_RC_TX
#define I2C_SCK 4
#define I2C_SDA 5
extern U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2;
extern U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2;
extern U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2;
extern int activeScreen;
extern boolean displayLocked;
extern byte menuRow;
#define MEM_SIZEOF(x) (sizeof(x) / sizeof(x[0]))
extern int addressDataVersion;
extern int addressDataVersion;
extern int addressTimeVersion;
extern int addressTimeVersion;
extern int addressVersion;
extern int addressReverse;
extern int addressNegative;
extern int addressPositive;
extern int activeChannelVulue;
#include "flash.h"
#include "readVCC.h"
#include "transmitterConfig.h"
#include "Variable.h"
#include "pong.h"
#include "wifiscann.h"

void setupRadio() ;
void setup() ;
void travelAdjust(boolean upDn) ;
void readButtons() ;
void JoystickOffset() ;
byte mapJoystick(byte input, byte arrayNo) ;
void readJoysticks() ;
void readPotentiometer() ;
void transmitRadio() ;
void readRadio() ;
void led() ;
void checkBattery() ;
void drawDisplay() ;
void drawTarget(int x, int y, int w, int h, int posX, int posY) ;
void loop() ;

#include "RC_Transmitter_u8g2_eeprom_base.ino"


#endif
