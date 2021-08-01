#include "DisplayDrivers.h"
#include "PasswordManager.h"
#include "Arduino.h"
#include "analogWrite.h"
#include <EEPROM.h>

unsigned long lastResetButtonPress = 0;
int incorrectTries = 0;

void SetupPasswordManager(bool alreadyInitialized)
{
  //Setup the output pins we want to use for our password methods.
  pinMode(PIN_RESET_PASSWORD_BUTTON, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  //Check if we have already been initialized once. If we have read out the previously stored amount of incorrect tries.
  if(alreadyInitialized) incorrectTries = EEPROM.read(INCORRECT_TRIES_ADDRESS);
}

bool ResetPasswordButtonPressed()
{
  if (digitalRead(PIN_RESET_PASSWORD_BUTTON)) 
  {
    //if 50ms have passed since last LOW pulse, it means that the
    //button has been pressed, released and pressed again.
    if (millis() - lastResetButtonPress > 200) 
    { 
      lastResetButtonPress = millis();   
      return true;
    }

    // Remember last button press event
    lastResetButtonPress = millis();
  }

  return false;
}


bool IsPasswordCorrect(int code[DIGIT_AMOUNT])
{
    //Read the stored code from memory. I currently use a single address for each digit. 
    //(since my values are only 0 - 9 I could have gone for 2 digits in each address with BCD encoding but was pressed for time as I wrote this last minute since other groupmates couldnt finish in time)
    int storedCode[DIGIT_AMOUNT];
    for(int i = 0; i < DIGIT_AMOUNT; i++)
    {
      storedCode[i] = EEPROM.read(STORED_PASSCODE_ADDRESS + i);
    }

    //Check if the code passed as argument matches the code stored in memory.
    for(int i = 0; i < DIGIT_AMOUNT; i++)
    {
        if(storedCode[i] != code[i])
        {
            incorrectTries++;
            EEPROM.write(INCORRECT_TRIES_ADDRESS, incorrectTries);
            EEPROM.commit();
            
            return false;
        }
    }
    
    incorrectTries = 0;
    return true;
}

int AmountOfCorrectTries()
{
    //Return the current amount of incorrect tries.
    Serial.print("Wrong Tries: ");
    Serial.println(incorrectTries);
    return incorrectTries;
}

void SetPasscode(int code[DIGIT_AMOUNT])
{
    //Write the passed code to memory to replace the old one.
    for(int i = 0; i < DIGIT_AMOUNT; i++)
    {
      Serial.println(code[i]);
      EEPROM.write(STORED_PASSCODE_ADDRESS + i, code[i]);
    }
    EEPROM.commit();
}

void SoundBuzzer()
{   
    //sound the buzzer for a predefined amont of time
    analogWrite(PIN_BUZZER, 128);
    delay(BUZZER_SOUND_TIME);
    analogWrite(PIN_BUZZER, 0);
}
