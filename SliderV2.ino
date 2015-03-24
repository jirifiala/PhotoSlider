#include <AFMotor.h>
#include <LiquidCrystal.h>



#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3
#define SELECT 4
#define NONE 5
#define KAMERA 0
#define FOTO 1
#define TOTALREWINDTIME 180000 // celkovy cas nez prejede z jedne strany na druhou
#define ONEREVTIME 22000 // jedno otoceni rotacni plochy
#define PRESSMOMENT 200 // debounce cas pro tlacitko
#define MINIMUMTIMETORUNROTATEMOTOR 10 // minimalni cas aby sepnul rotacni motor
LiquidCrystal lcd(48, 49, 44, 45, 46, 47);
AF_DCMotor pushMotor(4);
AF_DCMotor rotateMotor(2);
const int stopButtonFront(22);
const int stopButtonBack(24);
const int shutterRelay(23);
const int ledPhotoIndikator(26);
const int gradientThreadedRod(6); // spad zavitove tyce
const int RPM(55); // pocet otacek za minutu push motoru
int stopButtonFrontState;
int stopButtonBackState;
int buttonPressed = 0;
int button;


//-----------------------------------------------------------------------------------------------------------
//-------------------------------------------------SETUP-----------------------------------------------------
//-----------------------------------------------------------------------------------------------------------

void setup() {

	pinMode(stopButtonFront, INPUT);
	pinMode(stopButtonBack, INPUT);
	pinMode(shutterRelay, OUTPUT);
	pushMotor.setSpeed(255);
	rotateMotor.setSpeed(255);
	pushMotor.run(RELEASE);
	rotateMotor.run(RELEASE);
	lcd.begin(16, 2);
	lcd.print("Slider 2.0");
	pinMode(ledPhotoIndikator, OUTPUT);
	Serial.begin(115200); // baud rate of bluetooth module

}

//-----------------------------------------------------------------------------------------------------------
//-------------------------------------------------LOOP------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------


void loop() {


	double shutterTimesFractions[15] = { 100, 80, 60, 50, 40, 30, 25, 20, 15, 13, 10, 8, 6, 5, 4 };
	double shutterTimesSeconds[21] = { 0.3, 0.4, 0.5, 0.6, 0.8, 1, 1.3, 1.6, 2, 2.5, 3.2, 4, 5, 6, 8, 10, 13, 15, 20, 25, 30 };
	int shutterTimesFractionsSize = 15;
	int shutterTimessecondsSize = 21;


    int interval, sideMode, videoDuration, slideLength, rideStyle, rotateStyle,
		rotationEndAngle, rotationStartAngle, bulbDuration, bulbMode, rewind, directionForManual, direction, rotate, shutterSpeed;

	double rideDuration, timeToSlideOneMilimeter, stopsCount, stopDistance, timeToReachNextStop,
		revToSlideOneMilimeter, revTimeForOneTurn, sumOfOneOverN;

	/// <settings>


	/*/// rotation motor test
	while (1) {
	
		rotateMotor.run(FORWARD);
		delay(MINIMUMTIMETORUNROTATEMOTOR);
		rotateMotor.run(RELEASE);
		delay(500);
	}
	/// rotation motor test end*/
	

	sideMode = chooseSideMode(); // choose side mode automatic or manual

	if (sideMode == 0) {

		rewind = chooseSide(); // choose side
		direction = rewind;
		if (rewind == 1) {  // initialization of side
			rewindToHome();
		}
		else {
			rewindToEnd();
		}
	}
	else {

		initialPositionSet(); //  set position

		directionForManual = chooseDirectionForManual(); // set direction
		direction = directionForManual;
	}

	interval = intervalSet(); // set the interval between taking photos
	
	shutterSpeed = shutterSpeedSet(shutterTimesFractions, shutterTimesSeconds, shutterTimesFractionsSize, shutterTimessecondsSize); // set the interval between taking photos
	
    slideLength = slideLengthSet();  // set the slide legth
		
	rideStyle = rideStyleSet(); // set the ride style

	videoDuration = videoDurationSet(interval, slideLength, rideStyle); // set the duration of final timelapse video

	rotate = rotateDecision(); // choose if is rotation
	if (rotate == 2) {

		rotationEndAngle = rotationAngleSet(0); // set the rotation end angle

		rotationStartAngle = rotationAngleSet(rotationEndAngle); // set the rotation start angle

		rotateStyle = rotateStyleSet(); // set the ride style

	}
	else {

		rotationStartAngle = rotationEndAngle = rotateStyle = 1; // no rotation

	}
	bulbMode = bulbModeSet(); // bulb mode set

	bulbMode == 0 ? bulbDuration = 0 : bulbDuration = bulbDurationSet(); // bulb duration set

	/// </settings>

	/// <calculations>
	Serial.println("interval:");
	Serial.println(interval);
	rideDuration = interval * videoDuration * 25 * 1000;
	Serial.println(rideDuration);
	stopsCount = videoDuration * 25;
	Serial.println(stopsCount);

	revToSlideOneMilimeter = 1.0 / gradientThreadedRod;
	Serial.println(revToSlideOneMilimeter);

	revTimeForOneTurn = 60.0 / RPM;
	Serial.println(revTimeForOneTurn);

	timeToSlideOneMilimeter = revToSlideOneMilimeter * revTimeForOneTurn * 1000.0;
	Serial.println(timeToSlideOneMilimeter);

	stopDistance = (slideLength / stopsCount) * 10.0;
	Serial.println(stopDistance);

	timeToReachNextStop = stopDistance * timeToSlideOneMilimeter;
	Serial.println(timeToReachNextStop);

	sumOfOneOverN = 0;
	for (double i = 2; i <= (stopsCount + 2); i++) {
		sumOfOneOverN += 1.0 / i;
	}

	interval *= 1000;
	/// </calculations>

	/// <run>
	if ((rotationEndAngle - rotationStartAngle) != 0){ // ride with rotation motor

		double rotationAngle, oneRotationTime;

		rotationAngle = abs(rotationStartAngle - rotationEndAngle);

		int rotateDirection;

		if (rotationStartAngle - rotationEndAngle > 0) { // rotate direction
			rotateDirection = FORWARD;
		}
		else {
			rotateDirection = BACKWARD; 
		}

		int onerevtime = ONEREVTIME;

		double timeToRotateToNextStop;
		timeToRotateToNextStop = ((onerevtime / 360.0) * rotationAngle) / stopsCount;

		if (bulbMode) { // bulb is on
			rideWithRotationBulbOn(direction, timeToReachNextStop, rideStyle, interval, sumOfOneOverN, stopsCount, rotateStyle, timeToRotateToNextStop, rotateDirection, bulbDuration, slideLength);

		}
		else { // bulb is off
			rideWithRotationBulbOff(direction, timeToReachNextStop, rideStyle, interval, sumOfOneOverN, stopsCount, rotateStyle, timeToRotateToNextStop, rotateDirection, slideLength, shutterSpeed);

		}

	}
	else { // ride without rotation motor

		if (bulbMode) { // bulb is on

			rideWithoutRotationBulbOn(direction, timeToReachNextStop, rideStyle, interval, sumOfOneOverN, stopsCount, bulbDuration, slideLength);

		}
		else { // bulb is off

			Serial.println("WithoutRotationAndBulb");
			rideWithoutRotationAndBulb(direction, timeToReachNextStop, rideStyle, interval, sumOfOneOverN, stopsCount, slideLength, shutterSpeed);
		}

	}
	/// </run>

}
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
//-------------------------------------------------FUNCTIONS-DEFINITION--------------------------------------
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------
//------------------------------------------------CAREFULSLIDE-----------------------------------------------
//-----------------------------------------------------------------------------------------------------------

bool carefulSlide(int direction, double delayTime) {
	bool state = true;
	double delayTimeElaapsed = 0;
	Serial.println("CarefulSlide");
	if (direction == FORWARD) {
		stopButtonBackState = digitalRead(stopButtonBack);
		while (!stopButtonBackState && (delayTimeElaapsed < delayTime))
		{
			Serial.println("Forward");
			stopButtonBackState = digitalRead(stopButtonBack);
			pushMotor.run(FORWARD);
			delay(5);
			delayTimeElaapsed += 5;
		}
		pushMotor.run(RELEASE);
		if (stopButtonBackState) {
			return false;
		}
	}
	else if (direction == BACKWARD) {
		stopButtonFrontState = digitalRead(stopButtonFront);
		while (!stopButtonFrontState && (delayTimeElaapsed < delayTime))
		{
			Serial.println("Backward");
			stopButtonFrontState = digitalRead(stopButtonFront);
			pushMotor.run(BACKWARD);
			delay(5);
			delayTimeElaapsed += 5;
		}
		pushMotor.run(RELEASE);
		if (stopButtonFrontState) {
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------EDITEDARRAYOFTIMESTOREACHNEXTSTOP---------------------------------
//-----------------------------------------------------------------------------------------------------------

double* editedArrayOfTimesToReachNextStop(int style, int interval, double sumOfOneOverN, double TimeToReachNextStop, int stopsCount) {
	double* times;
	times = (double*)malloc((stopsCount + 1) * sizeof(double));

	double increment = 0;
	Serial.println("Times_Array:");

	if (style == 1) {
		for (int i = 0; i < stopsCount; i++) {
			times[i] = TimeToReachNextStop;
			Serial.println(i);
			Serial.println(TimeToReachNextStop);
		}
	}
	else if (style == 2) {
		for (int i = 0; i < stopsCount; i++) {
			double val;
			increment += 1.08 * ((1 / (i + 2.0)) / sumOfOneOverN);
			val = increment * TimeToReachNextStop;
			times[i] = val;
			Serial.println(i);
			Serial.println(times[i]);
		}
	}
	else if (style == 3) {
		for (int i = 0; i < stopsCount; i++) {
			double val;
			increment += 1.08 *  ((1 / (i + 2.0)) / sumOfOneOverN);
			val = increment * TimeToReachNextStop;
			times[(stopsCount - 1) - i] = val;
			Serial.println(i);
			Serial.println(times[i]);
		}
	}

	return times;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------EDITEDARRAYOFTIMESTOREACHNEXTSTOPNEW---------------------------------
//-----------------------------------------------------------------------------------------------------------

double* editedArrayOfTimesToReachNextStopNew(int style, double TimeToReachNextStop, int stopsCount, int interval, int slideLength) {
	double* times;
	times = (double*)malloc((stopsCount + 1) * sizeof(double));
	int max;
	max = 0;
	double function;
	Serial.println("Times_Array:");
	Serial.println("TimeToReachNextStop:");
	Serial.println(TimeToReachNextStop);

	if (style == 1) {
		
		for (int i = 0; i < stopsCount; i++) {
			times[i] = TimeToReachNextStop;
			Serial.println(i);
			Serial.println(TimeToReachNextStop);
		}
	}
	else if (style == 2) {
		stopsCount *= 1.75;
		for (int i = 0; i < stopsCount; i++) {
			double val;
			double x;
			double logfraction;
			double innerCount, exponent;
			x = sqrt(stopsCount) + i;
			exponent = sqrt(stopsCount);
			logfraction = x / exponent;
			innerCount = 1.0 + log(logfraction) / (x + exponent);
			function = pow(innerCount, exponent);
			function = function * 13 - 13.2;
			function = (function * (function / (1 / function)));
			val = function *TimeToReachNextStop;
			times[i] = val;
			Serial.println("Vypocet: ");
			Serial.println(i);
			Serial.println(times[i]);
			if (times[i] > max) {
				max = i;
			}
		}
	}
	else if (style == 3) {
		stopsCount *= 1.75;
		for (int i = 0; i < stopsCount; i++) {
			double val;
			double x;
			double logfraction;
			double innerCount, exponent;
			x = sqrt(stopsCount) + i;
			exponent = sqrt(stopsCount);
			logfraction = x / exponent;
			innerCount = 1.0 + log(logfraction) / (x + exponent);
			function = pow(innerCount, exponent);
			function = function * 13 - 13.2;
			function = (function * (function / (1 / function)));
			val = function *TimeToReachNextStop;
			times[(stopsCount - 1) - i] = val;
			Serial.println(i);
			Serial.println(function);
			Serial.println(times[i]);
			if (times[i] > max) {
				max = i;
			}
		}


	}

	if (times[max] > (interval)) {
		double constant;

		constant = (interval - 150) / times[max];

		for (int i = 0; i < stopsCount; i++) {
			times[i] = times[i] * constant;
		}

	}
	double sum = 0;
	for (int i = 0; i < stopsCount; i++) {
		sum += times[i];
	}
	double constant;
	constant = ((TOTALREWINDTIME*slideLength) / 100) / sum;

	for (int i = 0; i < stopsCount; i++) {
		times[i] = times[i] * constant;
	}

	return times;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------EDITEDARRAYOFTIMESTOREACHNEXTSTOPROTATION-------------------------
//-----------------------------------------------------------------------------------------------------------

double* editedArrayOfTimesToReachNextStopRotation(int style, double TimeToReachNextStop, int stopsCount, int interval) {
	double* times;
	times = (double*)malloc((stopsCount + 1) * sizeof(double));
	int max;
	max = 0;
	double function;
	Serial.println("Times_Array:");
	Serial.println("TimeToReachNextStop:");
	Serial.println(TimeToReachNextStop);

	if (style == 1) {

		for (int i = 0; i < stopsCount; i++) {
			times[i] = TimeToReachNextStop;
			Serial.println(i);
			Serial.println(TimeToReachNextStop);
		}
	}
	else if (style == 2) {
		stopsCount *= 1.75;
		for (int i = 0; i < stopsCount; i++) {
			double val;
			double x;
			double logfraction;
			double innerCount, exponent;
			x = sqrt(stopsCount) + i;
			exponent = sqrt(stopsCount);
			logfraction = x / exponent;
			innerCount = 1.0 + log(logfraction) / (x + exponent);
			function = pow(innerCount, exponent);
			function = function * 13 - 13.2;
			function = (function * (function / (1 / function)));
			val = function *TimeToReachNextStop;
			times[i] = val;
			Serial.println("Vypocet: ");
			Serial.println(i);
			Serial.println(times[i]);
			if (times[i] > max) {
				max = i;
			}
		}
	}
	else if (style == 3) {
		stopsCount *= 1.75;
		for (int i = 0; i < stopsCount; i++) {
			double val;
			double x;
			double logfraction;
			double innerCount, exponent;
			x = sqrt(stopsCount) + i;
			exponent = sqrt(stopsCount);
			logfraction = x / exponent;
			innerCount = 1.0 + log(logfraction) / (x + exponent);
			function = pow(innerCount, exponent);
			function = function * 13 - 13.2;
			function = (function * (function / (1 / function)));
			val = function *TimeToReachNextStop;
			times[(stopsCount - 1) - i] = val;
			Serial.println(i);
			Serial.println(function);
			Serial.println(times[i]);
			if (times[i] > max) {
				max = i;
			}
		}


	}

	if (times[max] > interval) {
		double constant;

		constant = (interval - 150) / times[max];

		for (int i = 0; i < stopsCount; i++) {
			times[i] = times[i] * constant;
		}

	}
	double sum = 0;
	for (int i = 0; i < stopsCount; i++) {
		sum += times[i];
	}
	
	for (int i = 0; i < stopsCount; i++) 
	{
		if (times[i] < MINIMUMTIMETORUNROTATEMOTOR) {
			times[i] += MINIMUMTIMETORUNROTATEMOTOR;
		}
	}

	return times;
}

//-----------------------------------------------------------------------------------------------------------
//----------------------------------------------RIDEWITHROTATIONBULBOFF--------------------------------------
//-----------------------------------------------------------------------------------------------------------

void rideWithRotationBulbOff(int motorDirection, double timeToReachNextStop, int rideStyle, int interval, double sumOfOneOverN, int stopsCount, int rotateStyle, double timeToRotateToNextStop, int rotationDirection, int slideLength, int shutterSpeed) {
	double *times = editedArrayOfTimesToReachNextStopNew(rideStyle, timeToReachNextStop, stopsCount, (interval - shutterSpeed), slideLength);
	double max = 0;
	for (int i = 0; i < stopsCount; i++)
	{
		if (times[i] > max) {
			max = times[i];
		}
	}
	double *timesForRotation = editedArrayOfTimesToReachNextStopRotation(rotateStyle, timeToRotateToNextStop, stopsCount, (interval-max-shutterSpeed));
	double totalSlideTimeRemaining;
	totalSlideTimeRemaining = ((interval/1000) * stopsCount) / 20;
	lcd.clear();

	for (int i = 0; i < stopsCount; i++) {
		lcdClearRow(lcd, 0);
		lcd.print("Zbyva: ");
		lcd.print(totalSlideTimeRemaining);
		lcd.print(" min");
		lcdClearRow(lcd, 1);
		lcd.print("Vyfoceno: ");
		lcd.print(i);
		double waitTime;
		waitTime = (interval) - times[i] - timesForRotation[i] - shutterSpeed;
		takePicture(0, shutterSpeed); 
		if (!carefulSlide(motorDirection, times[i])) {
			break;
		};

		rotateMotor.run(rotationDirection);
		delay(timesForRotation[i]);
		rotateMotor.run(RELEASE);
		delay(waitTime);
		totalSlideTimeRemaining = (interval / 1000) * (stopsCount - i) / 60;
	}

}

//-----------------------------------------------------------------------------------------------------------
//----------------------------------------------RIDEWITHROTATIONBULBON---------------------------------------
//-----------------------------------------------------------------------------------------------------------

void rideWithRotationBulbOn(int motorDirection, double timeToReachNextStop, int rideStyle, int interval, double sumOfOneOverN, int stopsCount, int rotateStyle, double timeToRotateToNextStop, int rotationDirection, int bulbDelay, int slideLength) {
	double *times = editedArrayOfTimesToReachNextStopNew(rideStyle, timeToReachNextStop, stopsCount, (interval-bulbDelay*1000), slideLength);
	double max = 0;
	for (int i = 0; i < stopsCount; i++)
	{
		if (times[i] > max) {
			max = times[i];
		}
	}
	double *timesForRotation = editedArrayOfTimesToReachNextStopNew(rotateStyle, timeToRotateToNextStop, stopsCount, (interval-max-bulbDelay*1000), slideLength);
	int totalSlideTimeRemaining;
	totalSlideTimeRemaining = (interval / 1000) * stopsCount / 60;
	lcd.clear();
	for (int i = 0; i < stopsCount; i++) {
		lcdClearRow(lcd, 0);
		lcd.print("Zbyva: ");
		lcd.print(totalSlideTimeRemaining);
		lcd.print(" min");
		lcdClearRow(lcd, 1);
		lcd.print("Vyfoceno: ");
		lcd.print(i);
		double waitTime;
		takePicture(bulbDelay,0);
		waitTime = (interval) - times[i] - timesForRotation[i] - bulbDelay * 1000;
		if (waitTime < 0) {
			break;
		}
		if (!carefulSlide(motorDirection, times[i])) {
			break;
		};

		rotateMotor.run(rotationDirection);
		delay(timesForRotation[i]);
		rotateMotor.run(RELEASE);
		delay(waitTime);
		totalSlideTimeRemaining = (interval / 1000) * (stopsCount - i) / 60;
	}

}

//-----------------------------------------------------------------------------------------------------------
//----------------------------------------------RIDEWITHOUTROTATIONANDBULB-----------------------------------
//-----------------------------------------------------------------------------------------------------------

void rideWithoutRotationAndBulb(int motorDirection, double timeToReachNextStop, int rideStyle, int interval, double sumOfOneOverN, int stopsCount, int slideLength, int shutterSpeed) {
	double *times = editedArrayOfTimesToReachNextStopNew(rideStyle, timeToReachNextStop, stopsCount, (interval-shutterSpeed), slideLength);
	int totalSlideTimeRemaining;
	totalSlideTimeRemaining = (interval / 1000) * stopsCount / 60;
	lcd.clear();
	for (int i = 0; i < stopsCount; i++) {
		lcdClearRow(lcd, 0);
		lcd.print("Zbyva: ");
		lcd.print(totalSlideTimeRemaining);
		lcd.print(" min");
		lcdClearRow(lcd, 1);
		lcd.print("Vyfoceno: ");
		lcd.print(i);
		double waitTime;
		waitTime = (interval) - times[i] - shutterSpeed;
		takePicture(0, shutterSpeed);
		if (!carefulSlide(motorDirection, times[i])) {
			Serial.println(i);
			break;
		};
		delay(waitTime);
		totalSlideTimeRemaining = (interval / 1000) * (stopsCount - i) / 60;
	}

}

//-----------------------------------------------------------------------------------------------------------
//----------------------------------------------RIDEWITHOUTROTATIOBULBON-------------------------------------
//-----------------------------------------------------------------------------------------------------------

void rideWithoutRotationBulbOn(int motorDirection, double timeToReachNextStop, int rideStyle, int interval, double sumOfOneOverN, int stopsCount, int bulbDelay, int slideLength) {
	double *times = editedArrayOfTimesToReachNextStopNew(rideStyle, timeToReachNextStop, stopsCount, (interval-bulbDelay*1000), slideLength);
	int totalSlideTimeRemaining;
	totalSlideTimeRemaining = (interval / 1000) * stopsCount / 60;
	lcd.clear();
	for (int i = 0; i < stopsCount; i++) {
		lcdClearRow(lcd, 0);
		lcd.print("Zbyva: ");
		lcd.print(totalSlideTimeRemaining);
		lcd.print(" min");
		lcdClearRow(lcd, 1);
		lcd.print("Vyfoceno: ");
		lcd.print(i);
		double waitTime;
		waitTime = (interval) - times[i] - bulbDelay * 1000;
		if (waitTime < 0) {
			break;
		}
		takePicture(bulbDelay,0); 
		if (!carefulSlide(motorDirection, times[i])) {
			break;
		};
		delay(waitTime);
		totalSlideTimeRemaining = (interval / 1000) * (stopsCount - i) / 60;
	}

}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------TAKEPICTURE-------------------------------------------
//-----------------------------------------------------------------------------------------------------------

void takePicture(int isBulb, int shutterSpeed)
{
	if (isBulb) {
		digitalWrite(shutterRelay, HIGH);
		delay(isBulb * 1000);
		digitalWrite(shutterRelay, LOW);
	}
	else {
		digitalWrite(shutterRelay, HIGH);
		delay(10);
		digitalWrite(shutterRelay, LOW);
		delay(shutterSpeed - 10);
	}

}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------REWINDHOME--------------------------------------------
//-----------------------------------------------------------------------------------------------------------

void rewindToHome() {
	pushMotor.setSpeed(255);
	lcd.clear();
	lcd.print("Pretacim...");
	stopButtonFrontState = digitalRead(stopButtonFront);
	while (!stopButtonFrontState)
	{
		stopButtonFrontState = digitalRead(stopButtonFront);
		pushMotor.run(BACKWARD);
		delay(10);
	}
	pushMotor.run(RELEASE);

}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------REWINDTOEND-------------------------------------------
//-----------------------------------------------------------------------------------------------------------

void rewindToEnd() {
	pushMotor.setSpeed(255);
	lcd.clear();
	lcd.print("Pretacim...");
	stopButtonBackState = digitalRead(stopButtonBack);
	delay(10);
	while (!stopButtonBackState)
	{

		stopButtonBackState = digitalRead(stopButtonBack);
		pushMotor.run(FORWARD);
		delay(5);
	}
	pushMotor.run(RELEASE);
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------WAITFORBUTTON-------------------------------------------
//-----------------------------------------------------------------------------------------------------------

int waitForButton() {
	int pressedButton = 5;
	button = NONE;
	int x;
	while (!buttonPressed) {
		delay(PRESSMOMENT);
		x = analogRead(0);
		if (x < 50) {
			pressedButton = RIGHT;
			buttonPressed = 1;
		}
		else if (x < 195) {
			pressedButton = UP;
			buttonPressed = 1;
		}
		else if (x < 380) {
			pressedButton = DOWN;
			buttonPressed = 1;
		}
		else if (x < 550) {
			pressedButton = LEFT;
			buttonPressed = 1;
		}
		else if (x < 790) {
			pressedButton = SELECT;
			buttonPressed = 1;
		}
	}
	buttonPressed = 0;
	return pressedButton;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------CHOOSESIDE-------------------------------------------
//-----------------------------------------------------------------------------------------------------------

int chooseSide() {
	button = NONE;
	lcd.clear();
	lcd.print("-----ZEPREDU----");
	lcd.setCursor(0, 1);
	lcd.print("     ZEZADU     ");
	int answer = 1;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == UP) {
			answer = 1;
			lcd.clear();
			lcd.print("-----ZEPREDU----");
			lcd.setCursor(0, 1);
			lcd.print("     ZEZADU     ");
		}
		else if (button == DOWN) {
			answer = 2;
			lcd.clear();
			lcd.print("     ZEPREDU    ");
			lcd.setCursor(0, 1);
			lcd.print("-----ZEZADU-----");
		}
	}

	return answer;
}


//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------ROTATEDICISION----------------------------------------
//-----------------------------------------------------------------------------------------------------------

int rotateDecision() {
	button = NONE;
	lcd.clear();
	lcd.print("---BEZ-ROTACE---");
	lcd.setCursor(0, 1);
	lcd.print("    S ROTACI    ");
	int answer = 1;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == UP) {
			answer = 1;
			lcd.clear();
			lcd.print("---BEZ-ROTACE---");
			lcd.setCursor(0, 1);
			lcd.print("    S ROTACI    ");
		}
		else if (button == DOWN) {
			answer = 2;
			lcd.clear();
			lcd.print("   BEZ ROTACE   ");
			lcd.setCursor(0, 1);
			lcd.print("----S-ROTACI----");
		}
	}

	return answer;
}
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------CHOOSEDIRECTIONFORMANUAL------------------------------
//-----------------------------------------------------------------------------------------------------------

int chooseDirectionForManual() {
	lcd.clear();
	lcd.print("-----DOPREDU----");
	lcd.setCursor(0, 1);
	lcd.print("     DOZADU     ");
	int answer = 2;
	button = NONE;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == UP) {
			answer = 2;
			lcd.clear();
			lcd.print("-----DOPREDU----");
			lcd.setCursor(0, 1);
			lcd.print("     DOZADU     ");
		}
		else if (button == DOWN) {
			answer = 1;
			lcd.clear();
			lcd.print("     DOPREDU    ");
			lcd.setCursor(0, 1);
			lcd.print("-----DOZADU-----");
		}
	}

	return answer;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------CHOOSESIDEMODE----------------------------------------
//-----------------------------------------------------------------------------------------------------------

int chooseSideMode() {
	lcd.clear();
	lcd.print("--AUTOMATICKY---");
	lcd.setCursor(0, 1);
	lcd.print("    MANUALNE    ");
	int answer = 0;
	button = NONE;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == UP) {
			answer = 0;
			lcd.clear();
			lcd.print("--AUTOMATICKY---");
			lcd.setCursor(0, 1);
			lcd.print("    MANUALNE    ");
		}
		else if (button == DOWN) {
			answer = 1;
			lcd.clear();
			lcd.print("  AUTOMATICKY   ");
			lcd.setCursor(0, 1);
			lcd.print("----MANUALNE----");
		}
	}

	return answer;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------BULBMODESET-------------------------------------------
//-----------------------------------------------------------------------------------------------------------

int bulbModeSet() {
	lcd.clear();
	lcd.print("----BULB-NE-----");
	lcd.setCursor(0, 1);
	lcd.print("    BULB ANO    ");
	int answer = 0;
	button = NONE;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == UP) {
			answer = 0;
			lcd.clear();
			lcd.print("----BULB-NE-----");
			lcd.setCursor(0, 1);
			lcd.print("    BULB ANO    ");
		}
		else if (button == DOWN) {
			answer = 1;
			lcd.clear();
			lcd.print("    BULB NE     ");
			lcd.setCursor(0, 1);
			lcd.print("----BULB-ANO----");
		}
	}

	return answer;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------ROTATIONANGLESET--------------------------------------
//-----------------------------------------------------------------------------------------------------------

int rotationAngleSet(int initialDegrees) {
	lcd.clear();
	if (initialDegrees == 0) {
		lcd.print("KONCOVE NATOCENI");
	}
	else {
		lcd.print("POCAT. NATOCENI");
	}
	rotateMotor.setSpeed(255);

	lcd.setCursor(0, 1);
	lcd.print("<>");
	button = NONE;
	double onerevtime;
	onerevtime = ONEREVTIME;
	double oneDegree;
	oneDegree = onerevtime / 360.0;
	int degrees = initialDegrees;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == LEFT) {
			degrees--;
			rotateMotor.run(FORWARD);
			delay(oneDegree);
			rotateMotor.run(RELEASE);
		}
		else if (button == RIGHT) {
			degrees++;
			rotateMotor.run(BACKWARD);
			delay(oneDegree);
			rotateMotor.run(RELEASE);
		}

	}

	return degrees;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------INITIALPOSITIONSET------------------------------------
//-----------------------------------------------------------------------------------------------------------

void initialPositionSet() {
	lcd.clear();
	lcd.print("STARTOVNI POZICE");
	lcd.setCursor(0, 1);
	lcd.print("<>");
	button = NONE;
	double totalDuration = TOTALREWINDTIME;
	int fiveCentimeters;
	fiveCentimeters = totalDuration / 20;
	int momentDelay;
	momentDelay = 0;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == LEFT) {
			stopButtonBackState = digitalRead(stopButtonBack);
			pushMotor.run(FORWARD);
			while ((momentDelay < fiveCentimeters) && !stopButtonBackState) {
				delay(10);
				stopButtonBackState = digitalRead(stopButtonBack);
				momentDelay += 10;
			}
			pushMotor.run(RELEASE);
			momentDelay = 0;
		}
		else if (button == RIGHT) {
			stopButtonFrontState = digitalRead(stopButtonFront);
			pushMotor.run(BACKWARD);
			while ((momentDelay < fiveCentimeters) && !stopButtonFrontState) {
				delay(10);
				stopButtonFrontState = digitalRead(stopButtonFront);
				momentDelay += 10;
			}
			pushMotor.run(RELEASE);
			momentDelay = 0;
		}

	}
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------VIDEODURATIONSET--------------------------------------
//-----------------------------------------------------------------------------------------------------------

int videoDurationSet(int interval, int slideLength, int rideStyle) {
	double rewindTime = TOTALREWINDTIME;
	double shortestTime;
	int shortestTimeInt;
	shortestTime = (((slideLength * rewindTime) / 100000) / 25) / interval;
	shortestTimeInt = shortestTime;
	if (rideStyle != 1) {
		shortestTimeInt *= 2;
		shortestTime++;
	}
	lcd.clear();
	lcd.print("DELKA VIDEA   <>");
	lcd.setCursor(0, 1);
	lcd.print(shortestTimeInt);
	lcd.print("s");
	button = NONE;
	int seconds = shortestTime;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == LEFT && seconds > shortestTimeInt) {
			seconds--;
			lcdClearRow(lcd, 1);
			lcd.print(seconds);
			lcd.print("s");
		}
		else if (button == RIGHT && seconds < 300) {
			seconds++;
			lcdClearRow(lcd, 1);
			lcd.print(seconds);
			lcd.print("s");
		}

	}

	return seconds;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------BULBDURATIONSET--------------------------------------
//-----------------------------------------------------------------------------------------------------------

int bulbDurationSet() {
	lcd.clear();
	lcd.print("BULB DURATION <>");
	lcd.setCursor(0, 1);
	button = NONE;
	int interval = 30;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == LEFT && interval > 30) {
			interval--;
			lcdClearRow(lcd, 1);
			lcd.print(interval);
			lcd.print("s");
		}
		else if (button == RIGHT && interval < 300) {
			interval++;
			lcdClearRow(lcd, 1);
			lcd.print(interval);
			lcd.print("s");
		}

	}

	return interval;
}//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------SHUTTERSPEEDSET----------------------------------------
//-----------------------------------------------------------------------------------------------------------

int shutterSpeedSet(double shutterTimesFractions[], double shutterTimesSeconds[], int shutterTimesFractionsSize,int shutterTimessecondsSize) {
	bool firstArray = true;
	int i, j = 0;
	lcd.clear();
	lcd.print("Zaverka      <>");
	lcd.setCursor(0, 1);
	lcd.print("1/");
	lcd.print(shutterTimesFractions[i]);
	button = NONE;
	
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == LEFT && i > 0) {
			if (j > 0) {
				j--;
				firstArray = false;
				lcdClearRow(lcd, 1);
				lcd.print(shutterTimesSeconds[j]);
				lcd.print("\"");
			}
			else {
				i--;
				firstArray = true;
				lcdClearRow(lcd, 1);
				lcd.print("1/");
				lcd.print(shutterTimesFractions[i]);
			}
			}
		else if (button == RIGHT && j< (shutterTimessecondsSize-1)) {
			if (i < shutterTimesFractionsSize) {
			i++;
			firstArray = true;
			lcdClearRow(lcd, 1);
			lcd.print("1/");
			lcd.print(shutterTimesFractions[i]);
			}
			else {
				if ((i == shutterTimesFractionsSize - 1)) {
					i++;
				}
				j++;
				firstArray = false;
				lcdClearRow(lcd, 1);
				lcd.print(shutterTimesSeconds[j]);
				lcd.print("\"");
			}

		}

	}
	if (firstArray) {
		return 1000 / shutterTimesFractions[i];
	}
	return shutterTimesSeconds[j]*1000;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------INTERVALSET-------------------------------------------
//-----------------------------------------------------------------------------------------------------------

int intervalSet() {
	lcd.clear();
	lcd.print("INTERVAL      <>");
	lcd.setCursor(0, 1);
	lcd.print("1s");
	button = NONE;
	int interval = 1;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == LEFT && interval > 1) {
			interval--;
			lcdClearRow(lcd, 1);
			lcd.print(interval);
			lcd.print("s");
		}
		else if (button == RIGHT && interval < 300) {
			interval++;
			lcdClearRow(lcd, 1);
			lcd.print(interval);
			lcd.print("s");
		}

	}

	return interval;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------SLIDELENGTHSET--------------------------------------
//-----------------------------------------------------------------------------------------------------------

int slideLengthSet() {
	lcd.clear();
	lcd.print("DELKA JIZDY   <>");
	lcd.setCursor(0, 1);
	lcd.print("100cm");
	button = NONE;
	int interval = 100;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == LEFT && interval > 5) {
			interval--;
			lcdClearRow(lcd, 1);
			lcd.print(interval);
			lcd.print("cm");
		}
		else if (button == RIGHT && interval < 100) {
			interval++;
			lcdClearRow(lcd, 1);
			lcd.print(interval);
			lcd.print("cm");
		}

	}

	return interval;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------LCDCLEARROW-------------------------------------------
//-----------------------------------------------------------------------------------------------------------

void lcdClearRow(LiquidCrystal lcd, int row) {
	lcd.setCursor(0, row);
	lcd.print("                ");
	lcd.setCursor(0, row);
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------RIDESTYLESET------------------------------------------
//-----------------------------------------------------------------------------------------------------------

int rideStyleSet() {
	lcd.clear();
	lcd.print("----LINEARNI----");
	lcd.setCursor(0, 1);
	lcd.print("  LOGARITMICKY  ");
	button = NONE;
	int styl = 1;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == UP && styl == 2) {
			styl--;
			lcd.clear();
			lcd.print("----LINEARNI----");
			lcd.setCursor(0, 1);
			lcd.print("  LOGARITMICKY  ");
		}
		else if (button == UP && styl == 3) {
			styl--;
			lcd.clear();
			lcd.print("    LINEARNI    ");
			lcd.setCursor(0, 1);
			lcd.print("--LOGARITMICKY--");
		}
		else if (button == DOWN && styl == 1) {
			styl++;
			lcd.clear();
			lcd.print("--LOGARITMICKY--");
			lcd.setCursor(0, 1);
			lcd.print("  EXPONENCIALNI ");
		}
		else if (button == DOWN && styl == 2) {
			styl++;
			lcd.clear();
			lcd.print("  LOGARITMICKY  ");
			lcd.setCursor(0, 1);
			lcd.print("--EXPONENCIALNI-");
		}


	}


	// styl == 1 - LINEARNI JIZDA
	// styl == 2 - LOGARITMICKA JIZDA
	// styl == 3 - EXPONENCIALNI JIZDA
	return styl;
}

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------ROTATESTYLESET----------------------------------------
//-----------------------------------------------------------------------------------------------------------

int rotateStyleSet() {
	lcd.clear();
	lcd.print("----LINEARNE----");
	lcd.setCursor(0, 1);
	lcd.print("  LOGARITMICKY  ");
	button = NONE;
	int styl = 1;
	while (button != SELECT)
	{
		button = waitForButton();
		if (button == UP && styl == 2) {
			styl--;
			lcd.clear();
			lcd.print("----LINEARNE----");
			lcd.setCursor(0, 1);
			lcd.print("  LOGARITMICKY  ");
		}
		else if (button == UP && styl == 3) {
			styl--;
			lcd.clear();
			lcd.print("    LINEARNE    ");
			lcd.setCursor(0, 1);
			lcd.print("--LOGARITMICKY--");
		}
		else if (button == DOWN && styl == 1) {
			styl++;
			lcd.clear();
			lcd.print("--LOGARITMICKY--");
			lcd.setCursor(0, 1);
			lcd.print("  EXPONENCIALNI ");
		}
		else if (button == DOWN && styl == 1) {
			styl++;
			lcd.clear();
			lcd.print("  LOGARITMICKY  ");
			lcd.setCursor(0, 1);
			lcd.print("--EXPONENCIALNI-");
		}


	}


	// styl == 1 - LINEARNI JIZDA
	// styl == 2 - LOGARITMICKA JIZDA
	// styl == 3 - EXPONENCIALNI JIZDA
	return styl;
}