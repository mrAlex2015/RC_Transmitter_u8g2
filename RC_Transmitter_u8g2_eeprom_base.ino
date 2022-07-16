/*
 * Micro RC Project. A tiny little 2.4GHz and LEGO "Power Functions" / "MECCANO" IR RC transmitter!
 * 3.3V, 8MHz Pro Mini
 * STM32F103C8T6 ARM version see: https://github.com/TheDIYGuy999/RC_Transmitter_STM32
 * 2.4GHz NRF24L01 radio module
 * SSD 1306 128 x 63 0.96" OLED
 * Custom PCB from OSH Park
 * Menu for the following adjustments:
 * -Channel reversing
 * -Channel travel limitation adjustable in steps of 5%
 * -Value changes are stored in EEPROM, individually per vehicle
 * Radio transmitter tester included (press "Select" button during power up)
 * NRF24L01+PA+LNA SMA radio modules with power amplifier are supported from board version 1.1
 * ATARI PONG game :-) Press the "Back" button during power on to start it
 *
 * Finally, I found a solution! I thought I'd post it here in case somebody else runs into the same problem.
 * The answer came from this link: https://github.com/Sloeber/arduino-eclipse-plugin/issues/10
 * GitHub it's for the Sloeber plugin but applies to the CDT.
 * The issue seems to be with the compiler indexes not being properly built.
 * Here:
 * TODO: При появлении ошибки отсутствия библиотечной функции или класса типа: Can not resolve Serial
 * Действия по устранению:
 * Setting the preference: Windows->Preferences->C/C++->Indexer->index unused headers.
 * Setting the preference: Windows->Preferences->C/C++->Indexer->index source and header files opened in the editor.
 * Setting the preference: Windows->Preferences->C/C++->Indexer->Files to index upfront . Add arduino.h and or WProgram.h.
 * Do next four in this sequence
 * Right click the Project->Index->Search for unresolved includes.
 * Right click the Project->Index->Freshen all Files.
 * Right click the Project->Index->Update with modified files.
 * Right click the Project->Index->Rebuild.
 * */

/*
 * TODO:
 * Выполнил установку выбора коммандного канала. Необходимо решить как изменить канал в приемнике
 * и выполнить переход на новый канал связи.
 *
 * Вариант:
 * После включения аппаратуры связь происходит на канале по умолчанию, после из схрона читаем номер и переходим на него!?
 * */

//
// =======================================================================================================
// BUILD OPTIONS (comment out unneeded options)
// =======================================================================================================
//
//#define DEBUG // if not commented out, Serial.print() is active! For debugging only!!
//#define OLED_DEBUG // if not commented out, an additional diagnostics screen is shown during startup
//
// =======================================================================================================
// INCLUDE LIRBARIES & TABS
// =======================================================================================================
//
// Libraries
#include "Arduino.h"
#include <SPI.h>
#include <Wire.h>
#include <RF24.h> // Installed via Tools > Board > Boards Manager > Type RF24
#include <printf.h>
#include <EEPROMex.h> // https://github.com/thijse/Arduino-EEPROMEx
#include <statusLED.h> // https://github.com/TheDIYGuy999/statusLED
#include <U8g2lib.h> // https://github.com/olikraus/u8glib
#include <avr/eeprom.h>

// Tabs (.h files in the sketch directory) see further down
const char codeCompileVersion[] PROGMEM = "AVR GCC: " __VERSION__ "."; // __GNUC__  __GNUC_MINOR__  __GNUC_PATCHLEVEL__
const char codeWelcome[] PROGMEM = "Welcome to the a2d test!\r\n";
const char codeNullString[] PROGMEM = "               \n";

const char codeCompileDetails[] PROGMEM = "@C: " __DATE__ "." __TIME__;
/**
 * IO definitions
 *
 * define access restrictions to peripheral registers
 * определения ограничения доступа к периферийным регистрам
 */
#define   __I   volatile const  /*!< "только чтение"  */
#define   __O   volatile        /*!< "только запись"  */
#define   __IO   volatile       /*!<  "чтение/запись" */

//Сохранение в EEPROM проинициализированной строки текста.
const char EEMEM eepromstrDate[14] = __DATE__;
const char EEMEM eepromstrTime[9] = __TIME__;
// Software revision
const float EEMEM codeVersion = 2.04;

struct TypeEEPROMStruct_dev {
	char const *dat;
	char const *tim;
	float const *ver;
};

struct TypeEEPROMStruct_dev MyStruct;

//#define EEPROMADR(x) (unsigned int) (&((TypeEEPROMStruct *)0x0000)->x)
#define BOOL_EEPROMADR(x) (unsigned int) ((bool *)&x)
#define UCHAR_EEPROMADR(x) (unsigned int) ((unsigned char *)&x)

//
// =======================================================================================================
// PIN ASSIGNMENTS & GLOBAL VARIABLES
// =======================================================================================================
//
// Is the radio or IR transmission mode active?
volatile byte transmissionMode = 1; // Radio mode is active by default

// Select operation trannsmitter operation mode
volatile byte operationMode = 0; // Start in transmitter mode (0 = transmitter mode, 1 = tester mode, 2 = game mode)

// Vehicle address
// Адрес транспортного средства
int vehicleNumber = 1; // Vehicle number one is active by default

// Radio channels (126 channels are supported)
//byte chPointer = 0; // Channel 1 (the first entry of the array) is active by default
//const byte NRFchannel[] { 110, 112 };
byte NRFchannel = 110;

// the ID number of the used "radio pipe" must match with the selected ID on the transmitter!
// 10 ID's are available @ the moment
const uint64_t pipeOut[] = {
/*    */0xE9E8F0F0B0LL,
/*    */0xE9E8F0F0B1LL,
/*    */0xE9E8F0F0B2LL,
/*    */0xE9E8F0F0B3LL,
/*    */0xE9E8F0F0B4LL,
/*    */0xE9E8F0F0B5LL,
/*    */0xE9E8F0F0B6LL,
/*    */0xE9E8F0F0B7LL,
/*    */0xE9E8F0F0B8LL,
/*    */0xE9E8F0F0B9LL
/*    */};

const int maxVehicleNumber = (sizeof(pipeOut) / (sizeof(uint64_t)));

// Hardware configuration: Set up nRF24L01 radio on hardware SPI bus & pins 7 (CE) & 8 (CSN)
RF24 radio(7, 8);

// Did the receiver acknowledge the sent data?
boolean transmissionState;

// LEGO powerfunctions IR
int pfChannel;
const int pfMaxAddress = 3;

// TX voltages
boolean batteryOkTx = false;
#define BATTERY_DETECT_PIN A7 // The 20k & 10k battery detection voltage divider is connected to pin A3
float txVcc;
float txBatt;

const byte EEMEM eeprom_joystickReversed[maxVehicleNumber + 1][4] = { //
		/*    */{ 0, 0, 0, 0 }, // Address 0 used for EEPROM initialisation
				{ 0, 0, 0, 0 }, // Address 1
				{ 0, 0, 0, 0 }, // Address 2
				{ 0, 0, 0, 0 }, // Address 3
				{ 0, 0, 0, 0 }, // Address 4
				{ 0, 0, 0, 0 }, // Address 5
				{ 0, 0, 0, 0 }, // Address 6
				{ 0, 0, 0, 0 }, // Address 7
				{ 0, 0, 0, 0 }, // Address 8
				{ 0, 0, 0, 0 }, // Address 9
				{ 0, 0, 0, 0 }, // Address 10
		};

//Joystick percent negative
const byte EEMEM eeprom_joystickPercentNegative[maxVehicleNumber + 1][4] = { //
		/*    */{ 100, 100, 100, 100 }, // Address 0 not used
				{ 100, 100, 100, 100 }, // Address 1
				{ 100, 100, 100, 100 }, // Address 2
				{ 100, 100, 100, 100 }, // Address 3
				{ 100, 100, 100, 100 }, // Address 4
				{ 100, 100, 100, 100 }, // Address 5
				{ 100, 100, 100, 100 }, // Address 6
				{ 100, 100, 100, 100 }, // Address 7
				{ 100, 100, 100, 100 }, // Address 8
				{ 100, 100, 100, 100 }, // Address 9
				{ 100, 100, 100, 100 }, // Address 10
		};

//Joystick percent positive
const byte EEMEM eeprom_joystickPercentPositive[maxVehicleNumber + 1][4] = { //
		/*    */{ 100, 100, 100, 100 }, // Address 0 not used
				{ 100, 100, 100, 100 }, // Address 1
				{ 100, 100, 100, 100 }, // Address 2
				{ 100, 100, 100, 100 }, // Address 3
				{ 100, 100, 100, 100 }, // Address 4
				{ 100, 100, 100, 100 }, // Address 5
				{ 100, 100, 100, 100 }, // Address 6
				{ 100, 100, 100, 100 }, // Address 7
				{ 100, 100, 100, 100 }, // Address 8
				{ 100, 100, 100, 100 }, // Address 9
				{ 100, 100, 100, 100 }, // Address 10
		};

//Joystick percent positive
const byte EEMEM eeprom_ChannelVulue[11] = { //
		/*    */100, // Address 0 not used
				100, // Address 1
				100, // Address 2
				100, // Address 3
				100, // Address 4
				100, // Address 5
				100, // Address 6
				100, // Address 7
				100, // Address 8
				100, // Address 9
				100  // Address 10
		};

// Joystick reversing
boolean joystickReversed[maxVehicleNumber + 1][4];

// Joystick percent negative
byte joystickPercentNegative[maxVehicleNumber + 1][4];

// Joystick percent positive
byte joystickPercentPositive[maxVehicleNumber + 1][4];

// Value channel
byte channelValue[maxVehicleNumber + 1];

// Joysticks
#define JOYSTICK_1 A1
#define JOYSTICK_2 A0
#define JOYSTICK_3 A3
#define JOYSTICK_4 A2

// Joystick push buttons
#define JOYSTICK_BUTTON_LEFT 4
#define JOYSTICK_BUTTON_RIGHT 2

byte leftJoystickButtonState;
byte rightJoystickButtonState;

// Buttons
#define BUTTON_LEFT   1  // - or channel select
#define BUTTON_RIGHT  10 // + or transmission mode select
#define BUTTON_SEL    0  // select button for menu
#define BUTTON_BACK   9  // back button for menu

byte leftButtonState = 7; // init states with 7 (see macro below)!
byte rightButtonState = 7;
byte selButtonState = 7;
byte backButtonState = 7;

// macro for detection of rising edge and debouncing
/*
 * the state argument (which must be a variable) records the current and the last 3 reads
 * by shifting one bit to the left at each read and bitwise anding with 15 (=0b1111).
 * If the value is 7(=0b0111) we have one raising edge followed by 3 consecutive 1's.
 * That would qualify as a debounced raising edge
 */
#define DRE(signal, state) (state=(((state<<1)|(signal&1))&15))==7

// Status LED objects (false = not inverted)
statusLED greenLED(false); // green: ON = ransmitter ON, flashing = Communication with vehicle OK
statusLED redLED(false); // red: ON = battery empty

#define HW_I2C_RC_TX

#ifdef HW_I2C_RC_TX

#define I2C_SCK 4
#define I2C_SDA 5

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* clock=*/I2C_SCK, /* data=*/
I2C_SDA, /* reset=*/
U8X8_PIN_NONE);
#endif

int activeScreen = 0; // the currently displayed screen number (0 = splash screen)
boolean displayLocked = true;
byte menuRow = 0; // Menu active cursor line

#define MEM_SIZEOF(x) (sizeof(x) / sizeof(x[0]))

// EEPROM (max. total size is 512 bytes)
// Always get the adresses first and in the same order
// Blocks of 11 x 4 bytes = 44 bytes each!

int addressDataVersion = EEPROM.getAddress(
		sizeof(char) * MEM_SIZEOF(eepromstrDate));
int addressTimeVersion = EEPROM.getAddress(
		sizeof(char) * MEM_SIZEOF(eepromstrTime));

int addressVersion = EEPROM.getAddress(sizeof(float));

int addressReverse = UCHAR_EEPROMADR(eeprom_joystickReversed); //EEPROM.getAddress(sizeof(byte) * 44);
int addressNegative = UCHAR_EEPROMADR(eeprom_joystickPercentNegative); //EEPROM.getAddress(sizeof(byte) * 44);
int addressPositive = UCHAR_EEPROMADR(eeprom_joystickPercentPositive); //EEPROM.getAddress(sizeof(byte) * 44);
int activeChannelVulue = UCHAR_EEPROMADR(eeprom_ChannelVulue);
//
// =======================================================================================================
// INCLUDE TABS (header files in sketch directory)
// =======================================================================================================
//
#include "flash.h"
#include "readVCC.h"
#include "transmitterConfig.h"
#include "Variable.h"
#include "pong.h"
#include "wifiscann.h"

//
// =======================================================================================================
// RADIO SETUP
// =======================================================================================================
//

void setupRadio() {
	radio.begin(); // инициируем работу модуля nRF24L01+
	//radio.setChannel(NRFchannel[chPointer]);
	radio.setChannel(NRFchannel);
//	radio.setAutoAck(1);        // режим подтверждения приёма, 1 вкл 0 выкл
//	radio.setRetries(0, 15); // (время между попыткой достучаться, число попыток)
//	radio.enableAckPayload(); // разрешить отсылку данных в ответ на входящий сигнал
	radio.powerUp();

	// Set Power Amplifier (PA) level to one of four levels: RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX
	radio.setPALevel(RF24_PA_MAX); // Independent NRF24L01 3.3 PSU, so "FULL" transmission level allowed
	radio.setDataRate(RF24_250KBPS);
	radio.setAutoAck(1, true);
//	radio.setAutoAck(pipeOut[vehicleNumber - 1], true); // Ensure autoACK is enabled
	radio.enableAckPayload();
	radio.enableDynamicPayloads();
	radio.setRetries(5, 5);        // 5x250us delay (blocking!!), max. 5 retries

#ifdef DEBUG
  radio.printDetails();
  delay(1800);
#endif

	// All axes to neutral position
	data.axis1 = 50;
	data.axis2 = 50;
	data.axis3 = 50;
	data.axis4 = 50;

	// Transmitter
	if (operationMode == 0) {
		radio.openWritingPipe(pipeOut[vehicleNumber - 1]); // Vehicle Number 1 = Array number 0, so -1!
		radio.write(&data, sizeof(RcData));
//		radio.powerUp();
//		radio.stopListening();
	}

	// Receiver (radio tester mode)
	if (operationMode == 1) {
		radio.openReadingPipe(1, pipeOut[vehicleNumber - 1]);
		radio.powerUp();
		radio.startListening();
	}

}

//
// =======================================================================================================
// MAIN ARDUINO SETUP (1x during startup)
// =======================================================================================================
//
void setup() {

	char lbuf[32];
	float f;

#ifdef DEBUG
	Serial.begin(115200);
	printf_begin();
#endif

	// LED setup
	greenLED.begin(6); // Green LED on pin 5
	redLED.begin(5); // Red LED on pin 6

	// Pinmodes (all other pinmodes are handled inside libraries)
	pinMode(JOYSTICK_BUTTON_LEFT, INPUT_PULLUP);
	pinMode(JOYSTICK_BUTTON_RIGHT, INPUT_PULLUP);
	pinMode(BUTTON_LEFT, INPUT_PULLUP);
	pinMode(BUTTON_RIGHT, INPUT_PULLUP);
	pinMode(BUTTON_SEL, INPUT_PULLUP);
	pinMode(BUTTON_BACK, INPUT_PULLUP);

	// EEPROM setup
	EEPROM.readBlock(addressReverse, joystickReversed); // restore all arrays from the EEPROM
	EEPROM.readBlock(addressNegative, joystickPercentNegative);
	EEPROM.readBlock(addressPositive, joystickPercentPositive);
	EEPROM.readBlock(activeChannelVulue, channelValue);
	// Display setup
	u8g2.setBusClock(100000); //Set the clock frequency of I2C to 100Khz
	u8g2.begin();

	// выбираем шрифт
	u8g2.setFontRefHeightExtendedText();
	u8g2.setFontPosBottom();
	u8g2.setFont(u8g_font_6x10);
	// очищаем память дисплея

	// draw the hourglass animation, full-half-empty
	u8g2.firstPage();  // вывод для графики
	do {
		u8g2.drawXBMP(34, 2, 60, 60, hourglass_full);
	} while (u8g2.nextPage()); // show display queue
	delay(500);

	u8g2.firstPage();  // вывод для графики
	do {
		u8g2.drawXBMP(34, 2, 60, 60, hourglass_half);
	} while (u8g2.nextPage()); // show display queue
	delay(500);

	u8g2.firstPage();  // вывод для графики
	do {
		u8g2.drawXBMP(34, 2, 60, 60, hourglass_empty);
	} while (u8g2.nextPage()); // show display queue
	delay(1000);

	MyStruct.dat = &eepromstrDate[0]; // инициализация cArray
	MyStruct.tim = &eepromstrTime[0]; // инициализация iNumber
	MyStruct.ver = &codeVersion; // инициализация uMinorVersion

	//Копирование строк из EEPROM в RAM.
	eeprom_read_block((void*) &lbuf, (const void*) &eepromstrDate,
			MEM_SIZEOF(eepromstrDate));
	eeprom_read_block((void*) &lbuf, (const void*) &eepromstrTime,
			MEM_SIZEOF(eepromstrTime));
	f = eeprom_read_float((const float*) &codeVersion);

	eeprom_read_block((void*) &joystickReversed,
			(const void*) &eeprom_joystickReversed,
			MEM_SIZEOF(eeprom_joystickReversed));

	eeprom_read_block((void*) &joystickPercentNegative,
			(const void*) &eeprom_joystickPercentNegative,
			MEM_SIZEOF(eeprom_joystickPercentNegative));

	eeprom_read_block((void*) &joystickPercentPositive,
			(const void*) &eeprom_joystickPercentPositive,
			MEM_SIZEOF(eeprom_joystickPercentPositive));

	eeprom_read_block((void*) &channelValue, (const void*) &eeprom_ChannelVulue,
			MEM_SIZEOF(eeprom_ChannelVulue));

// Switch to radio tester mode, if "Select" button is pressed
	if (digitalRead(BUTTON_BACK) && !digitalRead(BUTTON_SEL)) {
		operationMode = 1;
	}
// Switch to game mode, if "Back" button is pressed
	if (!digitalRead(BUTTON_BACK) && digitalRead(BUTTON_SEL)) {
		operationMode = 2;
	}

// Switch to game mode, if "Back" and "Select" button is pressed
	if (!digitalRead(BUTTON_BACK) && !digitalRead(BUTTON_SEL)) {
		operationMode = 4;
		// Receiver (radio tester mode)
		radio.begin(); // инициируем работу модуля nRF24L01+
		radio.setChannel(1);
		radio.powerUp();
		radio.setAutoAck(false);
		radio.startListening();
	} else {

		// Joystick setup
		JoystickOffset(); // Compute all joystick center points
		readJoysticks(); // Then do the first jocstick read

		// Radio setup
		setupRadio();
	}

// Splash screen
	checkBattery();
	//activeScreen = 0; // 0 = splash screen active
	drawDisplay();
#ifdef OLED_DEBUG
activeScreen = 100; // switch to the diagnostics screen
delay(1500);
drawDisplay();
#endif
	activeScreen = 1; // switch to the main screen
	delay(1500);
}

//
// =======================================================================================================
// BUTTONS
// =======================================================================================================
//
// Sub function for channel travel adjustment and limitation --------------------------------------
// Вспомогательная функция для регулировки и ограничения хода канала
void travelAdjust(boolean upDn) {
	byte inc = 5;
	if (upDn)
		inc = 5; // Direction +
	else
		inc = -5; // -

	if ((menuRow & 0x01) == 0) { // even (2nd column)
		joystickPercentPositive[vehicleNumber][(menuRow - 6) / 2] += inc; // row 6 - 12 = 0 - 3
	} else { // odd (1st column)
		joystickPercentNegative[vehicleNumber][(menuRow - 5) / 2] += inc; // row 5 - 11 = 0 - 3
	}

	joystickPercentPositive[vehicleNumber][(menuRow - 6) / 2] = constrain(
			joystickPercentPositive[vehicleNumber][(menuRow - 6) / 2], 20, 100);

	joystickPercentNegative[vehicleNumber][(menuRow - 5) / 2] = constrain(
			joystickPercentNegative[vehicleNumber][(menuRow - 5) / 2], 20, 100);
}

// Main buttons function --------------------------------------------------------------------------
void readButtons() {

// Every 10 ms
	signed char val;
	static unsigned long lastTrigger;
	if (millis() - lastTrigger >= 10) {
		lastTrigger = millis();

		// Left joystick button (Mode 1)
		if (DRE(digitalRead(JOYSTICK_BUTTON_LEFT), leftJoystickButtonState)
				&& (transmissionMode == 1)) {
			data.mode1 = !data.mode1;
			drawDisplay();
		}

		// Right joystick button (Mode 2)
		if (DRE(digitalRead(JOYSTICK_BUTTON_RIGHT), rightJoystickButtonState)
				&& (transmissionMode == 1)) {
			data.mode2 = !data.mode2;
			drawDisplay();
		}

		if (activeScreen <= 10) { // if menu is not displayed ----------
			// Left button: Channel selection
			if (DRE(digitalRead(BUTTON_LEFT), leftButtonState)
					&& (transmissionMode < 3)) {
				vehicleNumber++;
				if (vehicleNumber > maxVehicleNumber)
					vehicleNumber = 1;
				setupRadio(); // Re-initialize the radio with the new pipe address
				drawDisplay();
			}
		} else { // if menu is displayed -----------
				 // Left button: Value -
			if (DRE(digitalRead(BUTTON_LEFT), leftButtonState)) {
				if (activeScreen == 11) {
					joystickReversed[vehicleNumber][menuRow - 1] = false;
				}
				if (activeScreen == 12) {
					travelAdjust(false); // -
				}
				if (activeScreen == 155) {
					val = channelValue[vehicleNumber];
					if (val > 0)
						channelValue[vehicleNumber] = --val;
				}
				drawDisplay();
			}

			// Right button: Value +
			if (DRE(digitalRead(BUTTON_RIGHT), rightButtonState)) {
				if (activeScreen == 11) {
					joystickReversed[vehicleNumber][menuRow - 1] = true;
				}
				if (activeScreen == 12) {
					travelAdjust(true); // +
				}
				if (activeScreen == 155) {
					val = channelValue[vehicleNumber];
					if (val < 0x7F)
						channelValue[vehicleNumber] = ++val;
				}
				drawDisplay();
			}
		}

		// Menu buttons:

		// Select button: opens the menu and scrolls through menu entries
		if (DRE(digitalRead(BUTTON_SEL), selButtonState)
				&& (transmissionMode == 1)) {
			activeScreen = 11; // 11 = Menu screen 1
			menuRow++;
			if (menuRow > 4)
				activeScreen = 12; // 12 = Menu screen 2
			if (menuRow > 12) {
				//activeScreen = 11; // Back to menu 1, entry 1
				//menuRow = 1;
				activeScreen = 155;
			}
			if (menuRow > 12) {
				activeScreen = 11; // Back to menu 1, entry 1
				menuRow = 1;
			}
			drawDisplay();
		}

		// Back / Momentary button:
		if (activeScreen <= 10) { // Momentary button, if menu is NOT displayed
			;
			//if (!digitalRead(BUTTON_BACK))
			//	data.momentary1 = true;
			//else
			//	data.momentary1 = false;
		} else { // Goes back to the main screen & saves the changed entries in the EEPROM
			if (activeScreen == 155) {
				data.axis1 = 0;
				data.axis2 = 0;
				data.axis3 = 0;
				data.axis4 = 0;
				data.channel = channelValue[vehicleNumber];
				radio.write(&data, sizeof(struct RcData));
				delay(15);
				// TODO:
				NRFchannel = data.channel;
				radio.setChannel(NRFchannel);
				delay(1);
				byte temp = 10;
				while (temp--) {
					if (radio.isAckPayloadAvailable()) {
						radio.read(&payload, sizeof(struct ackPayload)); // read the payload, if available
						if (payload.channel == data.channel) {
							break; // отобразить на дисплее статус ответа, решаем что делать если что не так.
						}
					} else
						continue;
				}
			}

			if (DRE(digitalRead(BUTTON_BACK), backButtonState)) {
				activeScreen = 1; // 1 = Main screen
				menuRow = 0;
				drawDisplay();
				EEPROM.updateBlock(addressReverse, joystickReversed); // update changed values in EEPROM
				EEPROM.updateBlock(addressNegative, joystickPercentNegative);
				EEPROM.updateBlock(addressPositive, joystickPercentPositive);
				// TODO:
				EEPROM.updateBlock(activeChannelVulue, channelValue);
			}
		}
	}
}

//
// =======================================================================================================
// JOYSTICKS
// =======================================================================================================
//

int offset[4]; // the auto calibration offset of each joystick

// Auto-zero subfunction (called during setup) ----
void JoystickOffset() {
	offset[0] = 512 - analogRead(JOYSTICK_1);
	offset[1] = 512 - analogRead(JOYSTICK_2);
	offset[2] = 512 - analogRead(JOYSTICK_3);
	offset[3] = 512 - analogRead(JOYSTICK_4);
}

// Mapping and reversing subfunction ----
byte mapJoystick(byte input, byte arrayNo) {
	int reading[4];
	reading[arrayNo] = analogRead(input) + offset[arrayNo]; // read joysticks and add the offset
	reading[arrayNo] = constrain(reading[arrayNo], (1023 - range), range); // then limit the result before we do more calculations below

#ifdef CONFIG_2_CH // In most "car style" transmitters, less than a half of the throttle potentiometer range is used for the reverse. So we have to enhance this range!
if (reading[2] < (range / 2) ) {
	reading[2] = constrain(reading[2], (range / 3), (range / 2)); // limit reverse range, which will be multiplied later
	reading[2] = map(reading[2], (range / 3), (range / 2), 0, (range / 2));// reverse range multiplied by 4
}
#endif

	if (transmissionMode == 1 && operationMode != 2) { // Radio mode and not game mode
		if (joystickReversed[vehicleNumber][arrayNo]) { // reversed

			return map(reading[arrayNo], (1023 - range), range,
					(joystickPercentPositive[vehicleNumber][arrayNo] / 2 + 50),
					(50 - joystickPercentNegative[vehicleNumber][arrayNo] / 2));

		} else { // not reversed
			return map(reading[arrayNo], (1023 - range), range,
					(50 - joystickPercentNegative[vehicleNumber][arrayNo] / 2),
					(joystickPercentPositive[vehicleNumber][arrayNo] / 2 + 50));
		}
	} else { // IR mode
		return map(reading[arrayNo], (1023 - range), range, 0, 100);
	}
}

// Main Joystick function ----
void readJoysticks() {

// save previous joystick positions
	byte previousAxis1 = data.axis1;
	byte previousAxis2 = data.axis2;
	byte previousAxis3 = data.axis3;
	byte previousAxis4 = data.axis4;

// Read current joystick positions, then scale and reverse output signals, if necessary (only for the channels we have)
#ifdef CH1
	data.axis1 = mapJoystick(JOYSTICK_1, 0); // Aileron (Steering for car)
#endif

#ifdef CH2
	data.axis2 = mapJoystick(JOYSTICK_2, 1); // Elevator
#endif

#ifdef CH3
	data.axis3 = mapJoystick(JOYSTICK_3, 2); // Throttle
#endif

#ifdef CH4
	data.axis4 = mapJoystick(JOYSTICK_4, 3); // Rudder
#endif

// Only allow display refresh, if no value has changed!
	if (previousAxis1 != data.axis1 || previousAxis2 != data.axis2
			|| previousAxis3 != data.axis3 || previousAxis4 != data.axis4) {
		displayLocked = true;
	} else {
		displayLocked = false;
	}
}

//
// =======================================================================================================
// POTENTIOMETER
// =======================================================================================================
//
void readPotentiometer() {
	data.pot1 = map(analogRead(A6), 0, 1023, 0, 100);
	data.pot1 = constrain(data.pot1, 0, 100);
}

//
// =======================================================================================================
// TRANSMIT RADIO DATA
// =======================================================================================================
//
void transmitRadio() {

	static boolean previousTransmissionState;
	static float previousRxVcc;
	static float previousRxVbatt;
	static boolean previousBattState;
	static unsigned long previousSuccessfulTransmission;

	if (transmissionMode == 1) { // If radio mode is active: ----

		radio.flush_tx(); // Новая функция, которая стала доступной.

		// Send radio data and check if transmission was successful
		if (radio.write(&data, sizeof(struct RcData))) {
			if (radio.isAckPayloadAvailable()) {
				radio.read(&payload, sizeof(struct ackPayload)); // read the payload, if available
			}
			previousSuccessfulTransmission = millis();
		}

		radio.setChannel(NRFchannel);

		// if the transmission was not confirmed (from the receiver) after > 1s...
		if (millis() - previousSuccessfulTransmission > 1000) {
			greenLED.on();
			transmissionState = false;
			memset(&payload, 0, sizeof(payload)); // clear the payload array, if transmission error
#ifdef DEBUG
		Serial.println("Data transmission error, check receiver!");
#endif
		} else {
			greenLED.flash(30, 100, 0, 0); //30, 100
			transmissionState = true;
#ifdef DEBUG
		Serial.println("Data successfully transmitted");
#endif
		}

		if (!displayLocked) { // Only allow display refresh, if not locked ----
			// refresh transmission state on the display, if changed
			if (transmissionState != previousTransmissionState) {
				previousTransmissionState = transmissionState;
				drawDisplay();
			}

			// refresh Rx Vcc on the display, if changed more than +/- 0.05V
			if (payload.vcc - 0.05 >= previousRxVcc
					|| payload.vcc + 0.05 <= previousRxVcc) {
				previousRxVcc = payload.vcc;
				drawDisplay();
			}

			// refresh Rx V Batt on the display, if changed more than +/- 0.3V
			if (payload.batteryVoltage - 0.3 >= previousRxVbatt
					|| payload.batteryVoltage + 0.3 <= previousRxVbatt) {
				previousRxVbatt = payload.batteryVoltage;
				drawDisplay();
			}

			// refresh battery state on the display, if changed
			if (payload.batteryOk != previousBattState) {
				previousBattState = payload.batteryOk;
				drawDisplay();
			}
		}

#ifdef DEBUG
	Serial.print(data.axis1);
	Serial.print("\t");
	Serial.print(data.axis2);
	Serial.print("\t");
	Serial.print(data.axis3);
	Serial.print("\t");
	Serial.print(data.axis4);
	Serial.print("\t");
	Serial.println(F_CPU / 1000000, DEC);
#endif
	} else { // else infrared mode is active: ----
		radio.powerDown();
	}
}

//
// =======================================================================================================
// READ RADIO DATA (for radio tester)
// =======================================================================================================
//

void readRadio() {

	static unsigned long lastRecvTime = 0;
	byte pipeNo;

	payload.batteryVoltage = txBatt; // store the battery voltage for sending
	payload.vcc = txVcc; // store the vcc voltage for sending
	payload.batteryOk = batteryOkTx; // store the battery state for sending

	if (radio.available(&pipeNo)) {
		radio.read(&data, sizeof(struct RcData)); // read the radia data and send out the ACK payload
		radio.writeAckPayload(pipeNo, &payload, sizeof(struct ackPayload)); // prepare the ACK payload
		lastRecvTime = millis();
#ifdef DEBUG
	Serial.print(data.axis1);
	Serial.print("\t");
	Serial.print(data.axis2);
	Serial.print("\t");
	Serial.print(data.axis3);
	Serial.print("\t");
	Serial.print(data.axis4);
	Serial.println("\t");
#endif
	}

// Switch channel
	/*	if (millis() - lastRecvTime > 500) {
	 chPointer++;
	 if (chPointer >= sizeof((*NRFchannel) / sizeof(byte)))
	 chPointer = 0;
	 radio.setChannel(NRFchannel[chPointer]);
	 payload.channel = NRFchannel[chPointer];
	 }
	 */
	if (millis() - lastRecvTime > 1000) { // set all analog values to their middle position, if no RC signal is received during 1s!
		data.axis1 = 50; // Aileron (Steering for car)
		data.axis2 = 50; // Elevator
		data.axis3 = 50; // Throttle
		data.axis4 = 50; // Rudder
		payload.batteryOk = true; // Clear low battery alert (allows to re-enable the vehicle, if you switch off the transmitter)
#ifdef DEBUG
	Serial.println("No Radio Available - Check Transmitter!");
#endif
	}

	if (millis() - lastRecvTime > 2000) {
		setupRadio(); // re-initialize radio
		lastRecvTime = millis();
	}
}

//
// =======================================================================================================
// LED
// =======================================================================================================
//

void led() {

	// Red LED (ON = battery empty, number of pulses are indicating the vehicle number)
	if (batteryOkTx
			&& (payload.batteryOk || transmissionMode > 1 || !transmissionState)) {
		if (transmissionMode == 1)
			redLED.flash(140, 150, 500, vehicleNumber); // ON, OFF, PAUSE, PULSES
		if (transmissionMode == 2)
			redLED.flash(140, 150, 500, pfChannel + 1); // ON, OFF, PAUSE, PULSES
		if (transmissionMode == 3)
			redLED.off();
	} else {
		redLED.on(); // Always ON = battery low voltage (Rx or Tx)
	}
}

//
// =======================================================================================================
// CHECK TX BATTERY VOLTAGE
// =======================================================================================================
//

void checkBattery() {

// Every 500 ms
	static unsigned long lastTrigger;
	if (millis() - lastTrigger >= 500) {
		lastTrigger = millis();

#if F_CPU == 16000000 // 16MHz / 5V
	txBatt = (analogRead(BATTERY_DETECT_PIN) / 68.2) + diodeDrop; // 1023steps / 15V = 68.2 + diode drop!
#else // 8MHz / 3.3V
		txBatt = (analogRead(BATTERY_DETECT_PIN) / 103.33) + diodeDrop; // 1023steps / 9.9V = 103.33 + diode drop!
#endif
		txVcc = readVcc() / 1000.0;

		if (txBatt >= cutoffVoltage) {
			batteryOkTx = true;
#ifdef DEBUG
		Serial.print(txBatt);
		Serial.println(" Tx battery OK");
#endif
		} else {
			batteryOkTx = false;
#ifdef DEBUG
		Serial.print(txBatt);
		Serial.println(" Tx battery empty!");
#endif
		}
	}
}

//
// =======================================================================================================
// DRAW DISPLAY
// =======================================================================================================
//

void drawDisplay() {
//	bool box_fill = false;

	u8g2.firstPage();  // clear screen
	do {
		switch (activeScreen) {
		case 0: // Screen # 0 splash screen-----------------------------------

			if (operationMode == 0) {
				u8g2.setCursor(3, 10);
				u8g2.print(F("Micro RC Transmitter"));
			}
			if (operationMode == 1) {
				u8g2.setCursor(3, 10);
				u8g2.print(F("Micro RC Tester"));
			}
			if (operationMode == 2) {
				u8g2.setCursor(3, 10);
				u8g2.print(F("Micro PONG"));
			}
			if (operationMode == 4) {
				u8g2.setCursor(3, 10);
				u8g2.print(F("Micro NRF24 Scanner"));
			}
			// Dividing Line
			u8g2.drawLine(0, 13, 128, 13);

			// Software version
			u8g2.setCursor(3, 30);
			u8g2.print(F("SW: "));
			u8g2.print(codeVersion);
//			u8g.print(eepromfcodeVersion);

			// Hardware version
			u8g2.print(F(" HW: "));
			u8g2.print(boardVersion);

			u8g2.setCursor(3, 43);
			u8g2.print(F("created by:"));
			u8g2.setCursor(3, 55);
			u8g2.print(F("TheDIYGuy999"));

			break;

		case 100: // Screen # 100 diagnosis screen-----------------------------------
			u8g2.setCursor(3, 30);
			u8g2.print(F("Joystick readings:"));

			// Joysticks:
			u8g2.setCursor(3, 30);
			u8g2.print(F("Axis 1: "));
			u8g2.print(data.axis1);
			u8g2.setCursor(3, 40);
			u8g2.print(F("Axis 2: "));
			u8g2.print(data.axis2);
			u8g2.setCursor(3, 50);
			u8g2.print(F("Axis 3: "));
			u8g2.print(data.axis3);
			u8g2.setCursor(3, 60);
			u8g2.print(F("Axis 4: "));
			u8g2.print(data.axis4);

			break;

		case 1: // Screen # 1 main screen-------------------------------------

			// Tester mode ==================
			if (operationMode == 1) {
				// screen dividing lines ----
				u8g2.drawLine(0, 12, 128, 12);

				// Tx: data ----
				u8g2.setCursor(0, 10);
				u8g2.print(F("CH: "));
				u8g2.print(vehicleNumber);
				u8g2.setCursor(50, 10);
				u8g2.print(F("Bat: "));
				u8g2.print(txBatt);
				u8g2.print(F("V"));

				drawTarget(0, 14, 50, 50, data.axis4, data.axis3); // left joystick
				drawTarget(74, 14, 50, 50, data.axis1, data.axis2); // right joystick
				drawTarget(55, 14, 14, 50, 14, data.pot1); // potentiometer
			}

			// Transmitter mode ================
			if (operationMode == 0) {
				// screen dividing lines ----
				u8g2.drawLine(0, 13, 128, 13);
				u8g2.drawLine(64, 0, 64, 64);

				// Tx: data ----
				u8g2.setCursor(0, 10);
				if (transmissionMode > 1) {
					u8g2.print(F("Tx: IR   "));
					if (transmissionMode < 3)
						u8g2.print(pfChannel + 1);

					u8g2.setCursor(68, 10);
					if (transmissionMode == 2)
						u8g2.print(F("LEGO"));
					if (transmissionMode == 3)
						u8g2.print(F("MECCANO"));
				} else {
					u8g2.print(F("Tx: 2.4G"));
					u8g2.setCursor(52, 10);
					u8g2.print(vehicleNumber);
				}

				u8g2.setCursor(3, 25);
				u8g2.print(F("Vcc: "));
				u8g2.print(txVcc);
				/*
				 if (box_fill) {
				 box_fill = 0;
				 u8g2.setColorIndex(box_fill);
				 } else {
				 box_fill = 1;
				 u8g2.setColorIndex(box_fill);
				 }
				 u8g2.drawFrame(0, 34, 63, 10);
				 u8g2.setColorIndex(0);
				 */
				u8g2.setCursor(3, 35);
				u8g2.print(F("Bat: "));
				u8g2.print(txBatt);

				// Rx: data. Only display the following content, if in radio mode ----
				if (transmissionMode == 1) {
					u8g2.setCursor(68, 10);
					if (transmissionState) {
						u8g2.print(F("Rx: OK"));
					} else {
						u8g2.print(F("Rx: ??"));
					}

					u8g2.setCursor(3, 45);
					u8g2.print(F("Mode 1: "));
					u8g2.print(data.mode1);

					u8g2.setCursor(3, 55);
					u8g2.print(F("Mode 2: "));
					u8g2.print(data.mode2);

					if (transmissionState) {
						u8g2.setCursor(68, 25);
						u8g2.print(F("Vcc: "));
						u8g2.print(payload.vcc);

						u8g2.setCursor(68, 35);
						u8g2.print(F("Bat: "));
						u8g2.print(payload.batteryVoltage);

						u8g2.setCursor(68, 45);
						if (payload.batteryOk) {
							u8g2.print(F("Bat. OK "));
						} else {
							u8g2.print(F("Low Bat. "));
						}
						u8g2.setCursor(68, 55);
						u8g2.print(F("CH: "));
						u8g2.print(payload.channel);

						u8g2.setCursor(68, 65);
						u8g2.print(F("CH TX: "));
						u8g2.print(NRFchannel);
					}
				}
			}

			break;

		case 11: // Screen # 11 Menu 1 (channel reversing)-----------------------------------

			u8g2.setCursor(0, 10);
			u8g2.print(F("Channel Reverse ("));
			u8g2.print(vehicleNumber);
			u8g2.print(F(")"));

			// Dividing Line
			u8g2.drawLine(0, 13, 128, 13);

			// Cursor
			if (menuRow == 1)
				u8g2.setCursor(0, 25);
			if (menuRow == 2)
				u8g2.setCursor(0, 35);
			if (menuRow == 3)
				u8g2.setCursor(0, 45);
			if (menuRow == 4)
				u8g2.setCursor(0, 55);
			u8g2.print(F(">"));

			// Servos
			u8g2.setCursor(10, 25);
			u8g2.print(F("CH. 1 (R -): "));
			u8g2.print(joystickReversed[vehicleNumber][0]);	// 0 = Channel 1 etc.

			u8g2.setCursor(10, 35);
			u8g2.print(F("CH. 2 (R |): "));
			u8g2.print(joystickReversed[vehicleNumber][1]);

			u8g2.setCursor(10, 45);
			u8g2.print(F("CH. 3 (L |): "));
			u8g2.print(joystickReversed[vehicleNumber][2]);

			u8g2.setCursor(10, 55);
			u8g2.print(F("CH. 4 (L -): "));
			u8g2.print(joystickReversed[vehicleNumber][3]);

			break;

		case 12:// Screen # 12 Menu 2 (channel travel limitation)-----------------------------------

			u8g2.setCursor(0, 10);
			u8g2.print(F("Channel % - & + ("));
			u8g2.print(vehicleNumber);
			u8g2.print(F(")"));

			// Dividing Line
			u8g2.drawLine(0, 13, 128, 13);

			// Cursor
			if (menuRow == 5)
				u8g2.setCursor(45, 25);
			if (menuRow == 6)
				u8g2.setCursor(90, 25);
			if (menuRow == 7)
				u8g2.setCursor(45, 35);
			if (menuRow == 8)
				u8g2.setCursor(90, 35);
			if (menuRow == 9)
				u8g2.setCursor(45, 45);
			if (menuRow == 10)
				u8g2.setCursor(90, 45);
			if (menuRow == 11)
				u8g2.setCursor(45, 55);
			if (menuRow == 12)
				u8g2.setCursor(90, 55);
			u8g2.print(F(">"));

			// Servo travel percentage
			u8g2.setCursor(0, 25);
			u8g2.print(F("CH. 1:   "));
			u8g2.print(joystickPercentNegative[vehicleNumber][0]);// 0 = Channel 1 etc.
			u8g2.setCursor(100, 25);
			u8g2.print(joystickPercentPositive[vehicleNumber][0]);

			u8g2.setCursor(0, 35);
			u8g2.print(F("CH. 2:   "));
			u8g2.print(joystickPercentNegative[vehicleNumber][1]);
			u8g2.setCursor(100, 35);
			u8g2.print(joystickPercentPositive[vehicleNumber][1]);

			u8g2.setCursor(0, 45);
			u8g2.print(F("CH. 3:   "));
			u8g2.print(joystickPercentNegative[vehicleNumber][2]);
			u8g2.setCursor(100, 45);
			u8g2.print(joystickPercentPositive[vehicleNumber][2]);

			u8g2.setCursor(0, 55);
			u8g2.print(F("CH. 4:   "));
			u8g2.print(joystickPercentNegative[vehicleNumber][3]);
			u8g2.setCursor(100, 55);
			u8g2.print(joystickPercentPositive[vehicleNumber][3]);

			break;

		case 155:// Screen # 12 Menu 2 (channel travel limitation)-----------------------------------
			u8g2.setCursor(0, 10);
			u8g2.print(F("Set Channel ("));
			u8g2.print(channelValue[vehicleNumber - 1]);
			u8g2.print(F(")"));

			// Dividing Line
			u8g2.drawLine(0, 13, 128, 13);

			// Cursor
			u8g2.setCursor(0, 25);

			u8g2.print(F(">"));

			// Servos
			u8g2.setCursor(10, 25);
			u8g2.print(F("CHANNEL: "));
			u8g2.print(channelValue[vehicleNumber - 1]);// 0 = Channel 1 etc.
			break;
		}
//		u8g.setCursor(110, 10);
//		u8g.print(transmissionMode);
//		u8g.setCursor(118, 10);
//		u8g.print(operationMode);

	} while (u8g2.nextPage()); // show display queue
}

// Draw target subfunction for radio tester mode ----
void drawTarget(int x, int y, int w, int h, int posX, int posY) {
	u8g2.drawFrame(x, y, w, h);
	u8g2.drawDisc((x + w / 2) - (w / 2) + (posX / 2),
			(y + h / 2) + (h / 2) - (posY / 2), 5, 5);
}

//
// =======================================================================================================
//                   MAIN LOOP
// =======================================================================================================
//

void loop() {
	// Refresh display every 200 ms in tester mode (otherwise only, if value has changed)
	unsigned long lastDisplay;

	while (1) {
		// only read analog inputs in transmitter (0) or game mode (2)
		if (operationMode == 0 || operationMode == 2) {
			// Read joysticks
			readJoysticks();
			// Read Potentiometer
			readPotentiometer();
		}

		// Transmit data via infrared or 2.4GHz radio
		if (operationMode == 0) {
			transmitRadio(); // 2.4 GHz radio
		} else if (operationMode == 1) {
			readRadio(); // 2.4 GHz radio tester
		} else if (operationMode == 2) {
			pong();
		} else if (operationMode == 4) {
			scanerPrint();
		}

		if ((operationMode == 1) && ((millis() - lastDisplay) >= 200)) {
			lastDisplay = millis();
			drawDisplay();
		}

		if (operationMode != 2) {
			led(); // LED control
			checkBattery(); // Check battery
			readButtons();
		}
	}
}

