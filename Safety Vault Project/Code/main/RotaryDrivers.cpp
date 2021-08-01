#include "Arduino.h"
#include "RotaryDrivers.h"

unsigned long lastButtonPress = 0;
volatile int counter;
volatile bool flagA;
volatile bool flagB;

//Read the current state of both rotary encoder pins and map them into a single byte value.
byte ReadInputs()
{
    byte retVal = 0;

    retVal |= digitalRead(PIN_CLOCKWISE);
    retVal = retVal << 1;
    retVal |= digitalRead(PIN_COUNTERCLOCKWISE);

    return retVal;
}

//interrupt to be attached to pin A of the rotary encoder
void IRAM_ATTR PinA()
{
    byte val = ReadInputs(); //Get both states of the rotary encoder's pins

    //Check if we have already encountered flag B and if both pins are high.
    if(flagB && val == 3) //If we have this means we are rotating counter clockwise.
    {
      counter--; //decrement the counter. Also clamp the counter within its minimum and maximum range. (between 0 - 9)
      if(counter > RANGE_MAXIMUM_VALUE) counter = RANGE_MINIMUM_VALUE;
      if(counter < RANGE_MINIMUM_VALUE) counter = RANGE_MAXIMUM_VALUE;
      flagB = false;
      flagA = false;
    }
    else if(val == 2) //If we havent encountered flag B yet or not all pins are high, but the value corresponding with pin A is high, set flag A to true.
    {
      flagA = true;
    }
}

//interrupt to be attached to pin B of the rotary encoder
void IRAM_ATTR PinB()
{
    byte val = ReadInputs(); //Get both states of the rotary encoder's pins

    //Check if we have already encountered flag A and if both pins are high.
    if(flagA && val == 3) //If we have this means we are rotating counter clockwise.
    {
      counter++; //increment the counter. Also clamp the counter within its minimum and maximum range. (between 0 - 9)
      if(counter > RANGE_MAXIMUM_VALUE) counter = RANGE_MINIMUM_VALUE;
      if(counter < RANGE_MINIMUM_VALUE) counter = RANGE_MAXIMUM_VALUE;
      flagB = false;
      flagA = false;
    }
    else if(val == 1) //If we havent encountered flag A yet or not all pins are high, but the value corresponding with pin B is high, set flag B to true.
    {
      flagB = true;
    }
}

void SetupRotaryEncoder()
{
    //Setup the pins we need to use for the rotary encoder
    pinMode(PIN_BUTTON_PRESS, INPUT);
    pinMode(PIN_CLOCKWISE, INPUT);
    pinMode(PIN_COUNTERCLOCKWISE, INPUT);
    
    attachInterrupt(PIN_CLOCKWISE,PinA,FALLING); // set an interrupt on PinA, looking for a falling edge signal and executing the "PinA" Interrupt Service Routine (above)
    attachInterrupt(PIN_COUNTERCLOCKWISE,PinB,FALLING); // set an interrupt on PinB, looking for a falling edge signal and executing the "PinB" Interrupt Service Routine (above)
}

void UpdateRotaryEncoder(int *value, void (*onButtonPressCallback)())
{
    //Update the value reference we are receiving.
    *value = counter;
  
    //Check if the encoder's button is being pressed.
    if (digitalRead(PIN_BUTTON_PRESS)) 
    {
        //if 50ms have passed since last LOW pulse, it means that the
        //button has been pressed, released and pressed again
        if (millis() - lastButtonPress > 50) 
        {    
          onButtonPressCallback();
        }
    
        // Remember last button press event
        lastButtonPress = millis();
    }
}

//A small method called by ResetInput() to make sure the rotary encoder starts at 0 on a new digit or after entering a full code.
void ResetRotaryCounter()
{
  counter = 0;
}
