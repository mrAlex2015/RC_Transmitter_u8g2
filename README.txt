/*
 // объявляем структуру
 struct RevisionStruct {
 char eepromstrDate[12];
 char eepromstrTime[10];
 const float codeVersion = 2.04;
 };

 typedef struct {
 char eepromstrDate[16]; // массив символов
 char eepromstrTime[10]; // целое число
 float codeVersion;
 unsigned char uMinorVersion; // младший код версии
 unsigned char uMajorVersion; // старший код версии
 } TypeEEPROMStruct; // имя типа

 #define EEPROMADR(x) (unsigned int) (&((TypeEEPROMStruct *)0x0000)->x)

 #pragma memory=constseg(EEPROM) // Создать структуру в именованном сегменте
 TypeEEPROMStruct __EEPROMInitializationData = {
 //const TypeEEPROMStruct {
 __DATE__, / инициализация cArray
 __TIME__, / инициализация iNumber
 2.04, / инициализация uMinorVersion
 0x04, / инициализация uMajorVersion
 0x02 };
 */