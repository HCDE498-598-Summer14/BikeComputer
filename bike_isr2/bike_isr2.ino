/* Bike Computer Integration demo
ahdavidson
Jul 8, 2013

This program integrates all of the component demos for the bike computer prototype into one application.

To use it with your board, you should only have to change the constants that define the pin numbers for
where your components are connected on your board.

Note: you should not use pins digital 0 or 1 for your connections; they are reserved for the system.

This makes use of the serial monitor window to display text data, so open that after uploading the program
to see the data being printed.

*/


// these are necessary to use the LCD backpack and the RTC (clock) breakout board. they refer to external libraries
// other developers have created and that must get installed in your configuration of the IDE. they use a system
// called I2C, which requires two wires be connected on pins A4 and A5.

#include <Wire.h>    
#include <RTClib.h>
#include <LiquidCrystal.h> 


// constants that define pin numbers for different inputs and output
// change them to match your board's connections

const int CLKPin = A5;  // clock signal on I2C -- must be A5, don't change !
const int DATPin = A4;  // data signal for I2C -- must be A4, don't change !

const int ledPin  = 13;  // indicator LED -- usually on 13 since that is on the board also
const int tempPin = A1;  // temperature sensor -- MUST be an analog pin (A0 - A3; A4-A5 reserved for I2C)
const int magPin  = 2;   // magnetic (Hall Effect) sensor - any digital pin
const int btnPin  = 3;   // the pushbutton - any digital pin

const int magRadius = 700;  // distance from the center of the axle to the wheel rim (mm)

// we need to know the circumference of the wheel to turn RPMs into MPH. this calculation figured that
// in miles, based on the radius :  25.4 mm/in, 12 in/ft, 5280 ft/mile
const float wheelCircum = magRadius* 2 * PI / 25.4 / 12 / 5280;

// other constants

const byte degreeSym = 0xdf;  // the little degree symbol, built-in on the LCD

// temperature modes
const int fahrMode = 0; // just a name for the code number that means we want Fahrenheit temperatures
const int centMode = 1; // just a name for the code number that means we want Centigrade temperatures

// button actions
const int none       = 0;  // a name for the code that means "no action on the button"
const int pressed    = 1;  // a name for the code that means "button is just pressed briefly"
const int held       = 2;  // a name for the code that means "button is pressed and held for a certain time"

// button behavior
const int btnHoldTime = 1000;		// min time (ms) that constitutes a button being held rather than just pressed
const long debounceInterval = 10;	// delay to filter noise on button press (ms)


// this is used to access the LCD via the backpack - the zero means the LCD backpack is set to use I2C address "0"
// don't change that unless you change the hardware address on the backpack by soldering jumpers together.
// that would only be necessary if you had more than one LCD or another I2C device using address zero

LiquidCrystal lcd(0);


// below here are variables that we must be global {outside of the loop() and setup() routines} so that they can
// be "remembered" between iterations of the loop() code

int tempMode = fahrMode;	// which temperature mode we want displayed - default value is after "="

// these variables are used by the button routine to keep track of what's going on with it.
// you can just ignore them if you're aren't interested in the details. only a software masochist
// is interested in those details!

int     btnState = LOW;        // current state of button (HIGH/LOW)
int     btnPrev = LOW;         // prior state of button   (HIGH/LOW)
long    btnDown = false;       // last time button was detected down
long    btnUp = true;          // last time button was detected up
boolean btnIgnoreUp = false;   // true if waiting to see if button has been held down


// these variables are used by the magnetic sensor to calculate RPMs and other timing data

long time;
long prevRefreshTime = millis();
const int timeOut = 2000;

volatile long rev = 0;
long prevRev = 0;
int sumRev = 0;
float rpm;
float prevMPH = 0;


// now start things off

void setup() {

// setup the serial monitor window
  Serial.begin(9600);

// write an initial startup message out there (two blank lines, then the text)
  Serial.println ("\n\nBike Computer");

// set up the LCD and tell the software how many rows and columns it has
  lcd.begin(16, 2);

// and a message on the top line of the LCD
  lcd.print("Bike Computer");

// and some other fixed text so it doesn't have to be repeated each time in the main loop
  lcd.setCursor(4, 1); // position 5 ("4") on the second line ("1")
  lcd.print("MPH");
  lcd.setCursor(14, 1);
  lcd.write(degreeSym);  // this uses a built-in glyph on the LCD to show the degree symbol

// setup all the the pins for input or output
  pinMode(ledPin,  OUTPUT);
  pinMode(tempPin, INPUT);

// attach Interrupt for detecting a RISE in mag signal at pin 2 (for UNO) 
  attachInterrupt(0, isr, RISING);
}

void isr() //interrupt service routine to count revolutions
{
  rev++;
  digitalWrite(ledPin, HIGH);
}

// and do all of the rest of this continuously

void loop() {

// first, go check the temperature sensor and write the current Fahrenheit and Centigrade values out in the serial monitor

// read the raw value of the temp sensor
  int tempValue = analogRead(tempPin); 

// an analog sensor on the Arduino returns a value between 0 and 1023, depending on the strength
// of the signal it reads. (this is because the Arduino has a 10-bit analog to digital converter.)

// so we have to convert this value between 0-1023 into a temperature. the calculation is based on the
// voltage, and then the specs of the temperature sensor to convert that into degrees Celsius.

// if you're not interested in the details, you can just think of it as doing some math magic and skip down below

// first, convert the sensor reading to the right voltage level (5000 mV, since the Arduino port is +5V)
// the map function is just an efficient shortcut for some ratio math
  int tempMV = map(tempValue, 0, 1023, 0, 4999);

// now convert this voltage to degrees, both Fahrenheit and Centigrade
// this calculation comes from the spec of the specific sensor we are using
  float tempCent = (tempMV - 500) / 10.0;
  float tempFahr = (tempCent * 9 / 5.0) + 32;

// write these temperatures out to the serial monitor so we can see them
//  Serial.print(tempFahr);
//  Serial.print(" deg F  ");
//  Serial.print(tempCent);
//  Serial.print(" deg C");
//  Serial.println();

// and also to the LCD

// first format it as a whole number
  int tempDegrees;
  if (tempMode == fahrMode) {
    tempDegrees = round(tempFahr);
  } 
  else {
    tempDegrees = round(tempCent);
  } 



// TODO write comments for calculating speed


  digitalWrite(ledPin, LOW); 
  time = millis() - prevRefreshTime;
  sumRev = sumRev + (rev - prevRev);  // calculate the sum of revolutions taken in the refreshTime period
  prevRev = rev;
  
  if( sumRev > 1 || time > timeOut ) {

    rpm = ( (float)sumRev / time) * 60000;
    float mph = rpm * 60.0 * wheelCircum;
    
    Serial.print("Refreshed @ ");
    Serial.print(millis());
    Serial.print(": ");
    Serial.print(sumRev);
    Serial.print(" revolutions in ");
    Serial.print(time);
    Serial.print(" ms. Speed = ");
    Serial.print(mph);
    Serial.println(" MPH");

    lcd.setCursor(0, 1);
    lcd.print("   ");
    lcd.setCursor(0, 1);
    lcd.print((int)mph);
    
    sumRev = 0;
    prevRefreshTime = millis();
    prevMPH = mph;
    
    // then print it on the LCD with some fanciness
    // it will look like "42 *F" except that the asterisk will be a degree symbol
  
    lcd.setCursor(11, 1); // far right ("11") on the second line ("1")
    lcd.print("   ");
    lcd.setCursor(11, 1);	
    lcd.print(tempDegrees);
  
    lcd.setCursor(15, 1);
    if (tempMode == fahrMode) {
      lcd.print("F");
    } 
    else {
      lcd.print("C") ;
    }
  }


// finally, we check to see if the button has been pressed
// if it has, we just flip the mode for the temperature display from Fahrenheit to Centigrade or vice versa,
// depending which one is currently in effect

int btnAction = checkButtonAction(btnPin);
if (btnAction == pressed) {
// then we have to see which mode is currently in effect so we can flip it to the other one
  if (tempMode == fahrMode) {
    tempMode = centMode;
  } 
  else {
    tempMode = fahrMode;
  }
}


// and that's it for this time through the loop !

}


// this routine checks what is going on with the button connected on the pin passed to it ("pin").
// it should be called from loop() so that the status is updated as rapidly as possible to avoid latencies, i.e.
// user presses button but nothing happens. {it should also be made into an external library to hide its implementation.}

// it can detect a simple press or a press-and-hold event, defined by a global constant above (I know, I know...)

// it returns a code indicating the current state of the button. this code is one of:
//		none : nothing has happened -- the button has not been pressed
//		pressed : the button was pressed, so do what ever that means to the caller
//		held : the button was held down a certain minimum interval, then released

int checkButtonAction (int pin) {

// checks the given button status and returns a code indicating
// if it was just pressed, or if it was pressed and held for a
// given amount of time, or nothing has happened.

// this algorithm is adapted from Jeff Saltzman: jmsarduino.blogspot.com


// return value = button action code (none, pressed, held)
int result = none;  // if nothing else detected

// read the curent state of the button
btnState = digitalRead (pin);

// check to see if it just went down, with debouncing
if ((btnState == HIGH) &&
  (btnPrev == LOW) &&
  ((millis () - btnUp) > debounceInterval)) {

// button just went down for real, so just remember the time

  btnDown = millis ();
}

if ((btnState == LOW) &&
  (btnPrev == HIGH) &&
  ((millis () - btnUp) > debounceInterval)) {

// button was just released, so see how long it was down
// we are still waiting to see if it is being held down

  if (!btnIgnoreUp)
    result = pressed;
  else
    btnIgnoreUp = false;

  btnUp = millis ();
}

if ((btnState == HIGH) &&
  ((millis () - btnDown) > btnHoldTime)) {

// it's been down so long, call it held, not just pressed

  result = held;
btnIgnoreUp = true;
btnDown = millis ();
}

// remember this state for the next time around

btnPrev = btnState;

return result;

}






