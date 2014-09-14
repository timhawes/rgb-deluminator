#include <avr/sleep.h>
#include <PinChangeInt.h>

const int ledPin = 5;
const int armPin = 10;
const int dPin = 15;
const int triggerPin = 16;
const int IRLEDpin = 6;

const int BITtime = 562;
const int ledIdleLevel = 25;
const unsigned long sleepTimeout = 15000;

const unsigned long codes[] = {
  //0xFF906F, // red
  0xFF50AF, // blue  
  0xFF609F, // off
  0xFFE01F, // on
  0xFFC837  // smooth fade
};

volatile static unsigned long lastEvent = 0;
static unsigned int currentCode = 0;

void IRsetup(void)
{
  pinMode(IRLEDpin, OUTPUT);
  digitalWrite(IRLEDpin, LOW);    //turn off IR LED to start
}
 
// Ouput the 38KHz carrier frequency for the required time in microseconds
// This is timing critial and just do-able on an Arduino using the standard I/O functions.
// If you are using interrupts, ensure they disabled for the duration.
void IRcarrier(unsigned int IRtimemicroseconds)
{
  for(int i=0; i < (IRtimemicroseconds / 26); i++)
    {
    digitalWrite(IRLEDpin, HIGH);   //turn on the IR LED
    //NOTE: digitalWrite takes about 3.5us to execute, so we need to factor that into the timing.
    delayMicroseconds(9);          //delay for 13us (9us + digitalWrite), half the carrier frequnecy
    digitalWrite(IRLEDpin, LOW);    //turn off the IR LED
    delayMicroseconds(9);          //delay for 13us (9us + digitalWrite), half the carrier frequnecy
    }
}
 
//Sends the IR code in 4 byte NEC format
void IRsendCode(unsigned long code)
{
  //send the leading pulse
  IRcarrier(9000);            //9ms of carrier
  delayMicroseconds(4500);    //4.5ms of silence
  
  //send the user defined 4 byte/32bit code
  for (int i=0; i<32; i++)            //send all 4 bytes or 32 bits
    {
    IRcarrier(BITtime);               //turn on the carrier for one bit time
    if (code & 0x80000000)            //get the current bit by masking all but the MSB
      delayMicroseconds(3 * BITtime); //a HIGH is 3 bit time periods
    else
      delayMicroseconds(BITtime);     //a LOW is only 1 bit time period
     code<<=1;                        //shift to the next bit for this byte
    }
  IRcarrier(BITtime);                 //send a single STOP bit.
}

void gotosleep() {
  ADCSRA = 0;
  ACSR = (1<<ACD);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  sleep_disable(); 
}

void pinchanged() {
  lastEvent = millis();
}
  
void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(armPin, INPUT_PULLUP);
  pinMode(dPin, INPUT_PULLUP);
  pinMode(triggerPin, INPUT_PULLUP);
  
  IRsetup();

  PCintPort::attachInterrupt(armPin, &pinchanged, CHANGE);
  PCintPort::attachInterrupt(dPin, &pinchanged, CHANGE);
  PCintPort::attachInterrupt(triggerPin, &pinchanged, CHANGE);

  for (int i=0; i<255; i++) {
    analogWrite(ledPin, i);
    delay(1);
  }
  for (int i=255; i>=ledIdleLevel; i--) {
    analogWrite(ledPin, i);
    delay(1);
  }

}

void loop() {

  if (digitalRead(armPin) == LOW) {
    currentCode = (currentCode + 1) % (sizeof(codes)/sizeof(codes[0]));
    for (int i=0; i<currentCode+1; i++) {
      digitalWrite(ledPin, HIGH);
      delay(50);
      analogWrite(ledPin, ledIdleLevel);
      delay(150);
    }
    while (digitalRead(armPin)==LOW) 1;
    delay(50);
  }
  
  if (digitalRead(triggerPin) == LOW) {
    digitalWrite(ledPin, HIGH);
    IRsendCode(codes[currentCode]);
    analogWrite(ledPin, ledIdleLevel);
    delay(20);
  }
  
  if (millis() > lastEvent+sleepTimeout) {
    digitalWrite(ledPin, LOW);
    gotosleep();
    analogWrite(ledPin, ledIdleLevel);
  }
}

