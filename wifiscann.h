/*
 * wifiscann.h
 *
 *  Created on: 2 нояб. 2021 г.
 *      Author: Q
 */

#ifndef WIFISCANN_H_
#define WIFISCANN_H_

#define CHANNELS 128
#define NUM_EXPIRIENCE 128

byte count;
byte drawHeight;
uint8_t values[CHANNELS];

const char grey[] = " .:-=+*aRW";
const char filled = 'F';
const char drawDirection = 'R';
const char slope = 'W';

#define flipDisplay true

#define minRow       0
#define maxRow      127
#define minLine      0
#define maxLine     63

#define Row1         0
#define Row2        30
#define Row3        35
#define Row4        80
#define Row5        85
#define Row6        125

#define LineText    10
#define Line        12
#define LineVal     47

double multiplicator = 0.0;

/*
 * @description:
 * */
uint8_t MAX(volatile uint8_t *myArray) {
	uint8_t v;
	char maxVal = myArray[0];
	for (uint8_t i = 0; i < 128; i++) {
		if (myArray[i] > maxVal) {
			maxVal = myArray[i];
			v = i;
		}
	}
	return v;
}

/*
 * @description:
 * */
void getMultiplicator() {
	unsigned char maxVal = 1;
	for (int i = 0; i < maxRow; i++) {
		if (values[i] > maxVal)
			maxVal = values[i];
	}
	if (maxVal > LineVal)
		multiplicator = (double) LineVal / (double) maxVal;
	else
		multiplicator = 1;
}

/*
 * @description: Визуализация потока активности WiFi сигналов и самого мощного сигнала канала
 * */
void visualizeActiveWiFiChannelDisplay(void) {
	/*
	 * TODO:
	 * Имеется четыре разные точки отсчета позиции для текста:
	 * Слева вверху: setFontPosTop()
	 * Центр: setFontPosCenter()
	 * От базовой линии: setFontPosBaseline()
	 * Снизу: setFontPosBottom()
	 * */
	for (int i = 0; i < maxRow; i++) {
		values[i] = values[i + 1];
	}

	getMultiplicator();
	u8g2.firstPage();  // clear screen
	do {
		// maxVal = max(myArray[i],maxVal);
		// minVal = min(myArray[i],minVal);
		u8g2.drawLine(minRow, Line, maxRow, Line);
		u8g2.drawStr(Row1, LineText, "Ch:");
		uint8_t xx = MAX(values);
		u8g2.setCursor(24, LineText);
		u8g2.print(xx, DEC);
		u8g2.drawStr(60, LineText, "RPD=>-64dBm");
		u8g2.drawLine(xx, 18, xx, 14);
//		u8g.drawStr(Row2, LineText, (String)curChannel);
//		u8g.print(norm);

		for (int i = 0; i < maxRow; i++)
			u8g2.drawLine(i, maxLine, i, maxLine - values[i] * multiplicator);
	} while (u8g2.nextPage()); // show display queue
}

/*
 * @description:
 *
 * */
void printHead() {
	int i = 0;    // А это напечатает нам заголовки всех 127 каналов
	printf("\n\r");
	printf("/=== Scaning Channel number ===/\n");
//	printf("     ");
	while (i < 128) {
		printf("%x", i >> 4);
		++i;
	}
	printf("\n");
	i = 0;
//	printf("     ");
	while (i < 128) {
		printf("%x", i & 0xf);
		++i;
	}
	printf("\n\r");
}

/*
 * @description:
 * TODO:
 * bool radio.testCarrier() - true если есть несущая на канале установленном: radio.setChannel(номер канала)
 * bool radio.testRPD() - true если сигнал =>-64dBm (на модулях nrf24l01 без "+" не работает)
 * */
//
// =======================================================================================================
// SCANER
// Тело основного кода был перемещен ближе к main()- loop() программа адекватно заработала
// =======================================================================================================
//
void scanerPrint() {
	char mode = 0;
	char c = '\0';

	while (1) {
		// Send g over Serial to begin CCW output
		// Configure the channel and power level below
		if (Serial.available()) {
			c = Serial.read();
			if (c == 't') {             // stop
				radio.stopListening();
				mode = 2;
				Serial.println("Stop");
			} else if (c == 's') {      // start
				mode = 1;
				printHead();
				Serial.println("Start");
			} else if (c == 'p') {     // pause
				mode = 0;
				Serial.println("Pause");
			}
			c = '\0';
		}

		if (mode == 2)
			return;
//		if (mode == 1) {
		// Clear measurement values
		memset((void*) values, 0, sizeof(values));	//очищаем массив значений

		// Scan all channels num_reps times
		int rep_counter = NUM_EXPIRIENCE;// начальное значение счётчика приравниваем количеству прослушиваний
		// 16 экспериентов результат очень плохой сигналы еле заметные
		// 64 довольно не плохо
		// 86 практически ни чем не отличается от 64
		// 100
		// 156 получше особенно для ближайших сигналов но долго
		//делаем 100 экспериентов
		while (rep_counter--)//пока можно уменьшать счётчик прослушиваний (пока счётчик больше нуля)
		{
			int i = 127; // счётчик i  приравниваем 127 - количеству сканируемых каналов
			while (i--) // пока можно уменьшать i (пока i больше нуля) = пробегаемся по всем каналам
			{
				// Select this channel
				radio.setChannel(i); // устанавливаем номер канала

				// Listen for a little
				radio.startListening(); // переключаемся в режим приёмника
				delayMicroseconds(96); // слушаем эфир 225 микросекунд

				// Did we get a carrier? //было ли что-то услышано - получено на канале?
				if (radio.testCarrier()) { // если какие-то данные пришли
					++values[i]; //то прибавляем один к соответствующему элементу массива, отвечающему за количество раз, когда что-то было слышно на i-том канале
				}
				radio.stopListening(); //переходим в режим передатчика

			}
		}

		// Print out channel measurements, clamped to a single hex digit
		// распечатываем результаты эксперимента
		int i = 0;
		while (i < 128) {
			printf("%x", min(0xf, values[i] & 0xf));
			++i;
		}

		visualizeActiveWiFiChannelDisplay();
		Serial.println();
	}
}

/* void outputChannels(void) {
 int norm = 0;
 u8g.firstPage();  // clear screen
 do {
 //		for (int i = 0; i < CHANNELS; i++)
 //			if (values[i] > norm)
 //				norm = values[i];

 u8g.drawStr(84, 10, "Channel");
 u8g.setFontPosBottom();
 u8g.setFont(u8g_font_gdr10); //  u8g_font_9x15B
 u8g.setPrintPos(100, 30);
 u8g.print(norm);
 u8g.setFont(u8g_font_6x10);

 u8g.drawLine(0, 0, 0, 32);
 u8g.drawLine(80, 0, 80, 32);

 for (count = 0; count < 40; count += 10) {
 u8g.drawLine(80, count, 75, count);
 u8g.drawLine(0, count, 5, count);
 }

 for (count = 10; count < 80; count += 10) {
 u8g.drawPixel(count, 0);
 u8g.drawPixel(count, 31);
 }

 drawHeight = map(norm, 0, 200, 0, 32);
 sensorArray[0] = drawHeight;

 for (count = 1; count <= 80; count++) {
 if (filled == 'D' || filled == 'd') {
 if (drawDirection == 'L' || drawDirection == 'l') {
 u8g.drawPixel(count, 32 - sensorArray[count - 1]);
 } else //else, draw dots from right to left
 {
 u8g.drawPixel(80 - count, 32 - sensorArray[count - 1]);
 }
 } else {
 if (drawDirection == 'L' || drawDirection == 'l') {
 if (slope == 'W' || slope == 'w') {
 u8g.drawLine(count, 32, count,
 32 - sensorArray[count - 1]);
 } else {
 u8g.drawLine(count, 1, count,
 32 - sensorArray[count - 1]);
 }
 } else {
 if (slope == 'W' || slope == 'w') {
 u8g.drawLine(80 - count, 32, 80 - count,
 32 - sensorArray[count - 1]);
 } else {
 u8g.drawLine(80 - count, 1, 80 - count,
 32 - sensorArray[count - 1]);
 }
 }
 }
 }

 for (count = 80; count >= 2; count--) {
 sensorArray[count - 1] = sensorArray[count - 2];
 }
 } while (u8g.nextPage()); // show display queue
 }
 */

#endif /* WIFISCANN_H_ */
