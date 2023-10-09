#include <Arduino.h>
#include <Servo.h>
#include "SevSeg.cpp"

#define MIN_1 60000
#define PIN_ARM 9
#define PIN_DUNKER 10
#define BUTTON_START 13
#define BUTTON_SELECT 8
#define TIMER_DEFAULT 5
#define BRIGHTNESS 50
#define CLOCK_TICK 300  // higher uses less energy and is more stable, lower makes the stop button more responsive

SevSeg sevseg;
Servo servo_arm;
Servo servo_dunker;
int dunksBefore = 2;  // dunks before steeping
int dunksAfter = 3;  // dunks before steeping
int timer = TIMER_DEFAULT;    // minutes
unsigned long elapsedTimeMS = 0;
int lastButtonState = LOW;    // previous state of the button
bool lastSelectState = HIGH; // Assuming pull-up resistor, so unpressed is HIGH
bool lastStartState = HIGH;
bool currentSelectState;
bool currentStartState;

void numberSetup() {
  byte numDigits = 1;
  byte digitPins[] = {};
  byte segmentPins[] = {15, 16, 17, 18, 19, 21, 20};
  bool resistorsOnSegments = true;

  byte hardwareConfig = COMMON_CATHODE;
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(90);
}

/** Progressive time-based moves

    Instead of jerking the servo into the desired angle, use map()
    to turn a desired move time into angle measurements. In effect,
    this can be used to slow the servo down.
*/
void moveTo(int dest_angle, Servo *servo, int moving_time) {
  unsigned long progress = 0;
  unsigned long start_move = millis();
  int start_angle = servo->read();
  while (progress <= moving_time) {
    long angle = map(progress, 0, moving_time, start_angle, dest_angle);
    servo->write(angle);
    progress = millis() - start_move;
  }
}

void startPos(int time) {
  // Serial.println("start position");
  moveTo(0, &servo_dunker, time);
  moveTo(100, &servo_arm, time);
}

void bottomDunk(int timeArm, int timeDunker) {
  // Serial.println("bottom position");
  moveTo(70, &servo_arm, timeArm);
  moveTo(60, &servo_dunker, timeDunker);
}

void topDunk(int timeArm, int timeDunker) {
  // Serial.println("top position");
  moveTo(80, &servo_arm, timeArm);
  moveTo(20, &servo_dunker, timeDunker);
}

void dunk(int dunks, bool endBottom, int timeArm, int timeDunker) {
  // Serial.println("dunking");
  for (int i = 0; i < dunks; i++) {
    bottomDunk(timeArm, timeDunker);
    delay(100);
    topDunk(timeArm, timeDunker);
  }

  if (endBottom) {
    delay(100);
    bottomDunk(timeArm, timeDunker);
  }

}

void setup() {
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(BUTTON_START, INPUT_PULLUP);
  numberSetup();

  sevseg.setNumber(TIMER_DEFAULT);
  sevseg.setBrightness(BRIGHTNESS);

  servo_dunker.attach(PIN_ARM);
  servo_arm.attach(PIN_DUNKER);

  startPos(500);
}

void loop() {

  // time select
  while (true) {
    sevseg.refreshDisplay();
    // Serial.println("time select loop");
    currentSelectState = digitalRead(BUTTON_SELECT);
    currentStartState = digitalRead(BUTTON_START);

    // increment timer
    if(currentSelectState == LOW && lastSelectState == HIGH) { // button just pressed
      timer = (timer + 1) % 10;
      if (timer == 0)
        timer = 1;
      sevseg.setNumber(timer);
      sevseg.refreshDisplay();
      delay(200); // small delay to avoid multiple reads
    }

    // exit loop and start dunkin!
    if (currentStartState == LOW && lastStartState == HIGH){
      sevseg.blank();
      sevseg.refreshDisplay();
      delay(500);
      sevseg.setNumber(timer);
      sevseg.refreshDisplay();
      break;
    }

    lastSelectState = currentSelectState;
    lastStartState = currentStartState;
  }

  dunk(dunksBefore, true, 400, 1500);

  while(timer * MIN_1 > elapsedTimeMS) {
    // Serial.println("timer++");
    delay(CLOCK_TICK);
    currentStartState = digitalRead(BUTTON_START);
    elapsedTimeMS += CLOCK_TICK;
    // Serial.print("elapsed time: ");
    // Serial.println(elapsedTimeMS);

    // increment timer
    if (elapsedTimeMS % MIN_1 == 0) {
      // Serial.print("timer--");
      // Serial.print(timer);
      sevseg.setNumber(--timer);
      sevseg.refreshDisplay();
    }

    // check for button to end loop
    if (currentStartState == LOW && lastStartState == HIGH){
      timer = 0;
      sevseg.setNumber(0);
      sevseg.refreshDisplay();
      break;
    }
    lastStartState = currentStartState;
  }
  // Serial.println("finishing up!");

  dunk(dunksAfter, false, 200, 500);

  startPos(500);
}
