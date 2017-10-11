#include "Scale.h"

Scale *scale = NULL;
unsigned long startTime;
int state = 0;
bool isRunning = false;

void setup() 
{
    Serial.begin(9600);
    scale = new Scale();
    scale->connect();
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

    if (state == 0) {
      Serial.println("starting shot");
      if (scale->tare()) {
        scale->startTimer();
        state++;
      }
    }

    if (state == 1 && scale->getSeconds() >= 5) {
      Serial.println("stopping shot");
      scale->pauseTimer();
      state++;
      startTime = millis();
    }

    if (state == 2 && (startTime + 5000) < millis()) {
      scale->stopTimer();
      //scale->disconnect();
      state++;
    }
  }

  delay(1);
}


