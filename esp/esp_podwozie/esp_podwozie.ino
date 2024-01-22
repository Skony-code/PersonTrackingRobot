#include "Arduino.h"

//motors pins
//left
const int enA = 19;
const int out1 = 18;
const int out2 = 17;
//right
const int enB = 32;
const int out3 = 25;
const int out4 = 33;
//in

const int in3 = 13;
const int in2 = 27;
const int in1 = 26;
//motors controls
#define RIGHT_MOTORS 0
#define LEFT_MOTORS 1

#define FORWARD 0
#define BACKWARD 1
#define STOP 2

void rotateMotors(int side, int direction, int speed/*0-255*/) {
  if(speed>1) {
    speed = 106;
  }
  if(side == LEFT_MOTORS) {
      if(direction == FORWARD) {
        digitalWrite(out1,HIGH);
        digitalWrite(out2,LOW);
      }
      else if(direction == BACKWARD) {
        digitalWrite(out1,LOW);
        digitalWrite(out2,HIGH);
      }
      else if(direction == STOP) {
        digitalWrite(out1,LOW);
        digitalWrite(out2,LOW);
      }
      analogWrite(enA,speed);
  }
  else if(side == RIGHT_MOTORS) {
      if(direction == FORWARD) {
        digitalWrite(out3,HIGH);
        digitalWrite(out4,LOW);
      }
      else if(direction == BACKWARD) {
        digitalWrite(out3,LOW);
        digitalWrite(out4,HIGH);
      }
      else if(direction == STOP) {
        digitalWrite(out3,LOW);
        digitalWrite(out4,LOW);
      }
    analogWrite(enB,speed);
  }
}

void rotateLeft(int speed/*0-255*/) {
  rotateMotors(RIGHT_MOTORS,FORWARD,speed);
  rotateMotors(LEFT_MOTORS,BACKWARD,speed);
}

void rotateRight(int speed/*0-255*/) {
  rotateMotors(RIGHT_MOTORS,BACKWARD,speed);
  rotateMotors(LEFT_MOTORS,FORWARD,speed);
}

void moveForward(int speed/*0-255*/) {
  rotateMotors(RIGHT_MOTORS,FORWARD,speed);
  rotateMotors(LEFT_MOTORS,FORWARD,speed);
}

void moveBackward(int speed/*0-255*/) {
  rotateMotors(RIGHT_MOTORS,BACKWARD,speed);
  rotateMotors(LEFT_MOTORS,BACKWARD,speed);
}

void stop() {
  rotateMotors(RIGHT_MOTORS,STOP,0);
  rotateMotors(LEFT_MOTORS,STOP,0);
}



void setup() {
  pinMode(enA, OUTPUT);
	pinMode(enB, OUTPUT);
	pinMode(out1, OUTPUT);
	pinMode(out2, OUTPUT);
	pinMode(out3, OUTPUT);
	pinMode(out4, OUTPUT);

  pinMode(in1, INPUT);
	pinMode(in2, INPUT);
	pinMode(in3, INPUT);
}

void loop() {
  Serial.begin(921600);
  Serial.println();

  int in1_val = digitalRead(in1);
  int in2_val = digitalRead(in2);
  int in3_val = digitalRead(in3);
  //int pwmOutput = map(in3_val, 0, 4096, 0 , 255); 
  //Serial.println(in3_val);
  if(in3_val) {
    if(in1_val) rotateMotors(RIGHT_MOTORS,FORWARD,100);
    else rotateMotors(RIGHT_MOTORS,BACKWARD,100);
    if(in2_val) rotateMotors(LEFT_MOTORS,FORWARD,100);
    else rotateMotors(LEFT_MOTORS,BACKWARD,100);
  } else {
    stop();
  }
  delay(10);
}