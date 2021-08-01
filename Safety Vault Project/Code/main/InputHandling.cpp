#include "Arduino.h"
#include "DisplayDrivers.h"
#include "InputHandling.h"
#include "RotaryDrivers.h"

int currentRotValue = 0;
DisplayData *_dataPointer;
void (*completedInputCallback)();

#define READ_INPUT_DELAY 50
unsigned long timeSinceRead = 0;

void SubmitDigit()
{
    //Check if we are currently flasing the display, if so we dont need to update our dot position.
    if(_dataPointer->displayIsFlashing) return;

    //Reset values
    currentRotValue = 0;
    ResetRotaryCounter();
    _dataPointer->dotPosition++;

    //Check if we've entered 4 digits, if so we have a full code.
    if(_dataPointer->dotPosition > 3) 
    {
        completedInputCallback();
        _dataPointer->dotPosition = 0;
    }
}

void SetupInputHandler(DisplayData *_data, void (*onCompletedInputCallback)())
{
    //Store pointer for later use and setup callback & rotary encoder
    _dataPointer = _data;
    completedInputCallback = onCompletedInputCallback;
    SetupRotaryEncoder();
}

void HandleInput()
{
    //Update the rotary encoder
    UpdateRotaryEncoder(&currentRotValue, &SubmitDigit);

    //Check if we aren't flashing the display
    if(!_dataPointer->displayIsFlashing)
    {
        //If we arent, wait 50ms to read the current value (to circumvent double reading due to the callbacks)
        if(millis() - timeSinceRead > READ_INPUT_DELAY)
        {
          timeSinceRead = millis();
          _dataPointer->digits[_dataPointer->dotPosition] = currentRotValue;
        }
    }
    else //If we are, just constantly reset the input. (We dont want the user to be able to input anything while the display is flashing.
    {
        ResetInput();
    }
}

void ResetInput()
{
  //Reset digit values to 0.
  _dataPointer->digits[0] = 0;
  _dataPointer->digits[1] = 0;
  _dataPointer->digits[2] = 0;
  _dataPointer->digits[3] = 0;

  //Reset the dot position and the rotary encoder's position.
  _dataPointer->dotPosition = 0;
  ResetRotaryCounter();
}
