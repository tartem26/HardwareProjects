#include <ESP32Servo.h>
#include "DoorDrivers.h"

Servo servo;

void SetupDoorDrivers()
{
  pinMode(PIN_SERVO, OUTPUT);
  pinMode(PIN_DOORSTATE, INPUT);

  servo.setPeriodHertz(50); 

  servo.attach (PIN_SERVO);
  //set the servo physically to the LOCKED position
  servo.write(LOCKED);
}

void Lock()
{ 
    servo.write(LOCKED);
}

void Unlock()
{
    servo.write(UNLOCKED);
}

bool IsDoorOpen()
{
  return !digitalRead(PIN_DOORSTATE);
}
