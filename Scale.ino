#include "Scale.h"

Scale *scale = NULL;
unsigned long start;
bool isTare = false;
bool isRunning = false;

void setup() 
{
    Serial.begin(9600);
    scale = new Scale();
    scale->connect();

    start = millis();
}

void loop() {
  
  scale->update();
  if (scale->hasWeightChanged()) {
    Serial.print("battery: ");
    Serial.println(scale->getBattery());
    Serial.print("weight: ");
    Serial.println(scale->getWeight());
    Serial.print("seconds: ");
    Serial.println(scale->getSeconds());

    if (!isTare) {
      Serial.println("starting shot");
      isTare = scale->tare();
      isRunning = scale->startTimer();
    }

    if (isRunning && scale->getSeconds() >= 10) {
      Serial.println("stopping shot");
      scale->stopTimer();
      isRunning = false;
    }
  }

  delay(1);
}

