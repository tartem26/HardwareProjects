#include "DisplayDrivers.h"
#include "InputHandling.h"
#include "DoorDrivers.h"
#include "PasswordManager.h"
#include "BluetoothHandler.h"
#include <EEPROM.h>

#define EEPROM_SIZE 512
#define INIT_FLAG_ADDRESS 0
#define VAULT_STATE_ADDRESS INCORRECT_TRIES_ADDRESS + 1

#define BLUETOOTH_ENABLED false

DisplayData _data; //This variable is shared between all methods that need to use the display. The display drivers then use this data to display the correct digits etc.
unsigned long timeWhenDoorUnlocked = 0; 
unsigned long timeWhenDoorLocked = 0;
unsigned long timeWhenDoorPhysicallyClosed = 0;
int previousPhysicalDoorState;
bool doorWasClosed = false;

bool buzzerIsSounding = false;
unsigned long buzzerStartedTime;

enum VaultState
{
  AcceptingInput = 0,
  InputLocked = 1
};
VaultState currentVaultState;

enum InputHandlerTask
{
    EnteringCode,
    ChangingPassword
};
InputHandlerTask currentInputTask;

enum DoorState
{
    Locked,
    Unlocked
};
DoorState currentDoorState;

void setup() 
{
    //Initialize FLASH memory & Serial communication
    EEPROM.begin(EEPROM_SIZE);
    Serial.begin(115200);

    //Initialize the Input, Display & Door drivers.
    SetupInputHandler(&_data, &ReceivedInput);
    SetupDisplayTask(&_data);
    SetupDoorDrivers();

    bool initialized = false;

    //Check if our program has been initialized before (will always be true unless the chip was just reprogrammed)
    if(EEPROM.read(INIT_FLAG_ADDRESS) == 1) //The program has already been initialized once. This means that the ESP32 lost power and has rebooted. No need to change password.
    {       
        initialized = true; //Set initialized to true. Used later to setup the password manager
        
        Serial.println("Already Initialized");

        //Set states to their stored or default states.
        currentInputTask = EnteringCode;
        currentVaultState = (VaultState)EEPROM.read(VAULT_STATE_ADDRESS);

        Serial.println("Vault already initialized, now accepting input.");
    } 
    else //The program has not been started before. This means we wont have a password so we need to ask for one.
    {
        Serial.println("Initializing");

        //Write to memory that we have now initialized our program. We dont need to do this again on the next restart.
        EEPROM.write(INIT_FLAG_ADDRESS, 1); 

        //Set states to the default startup states. 
        currentInputTask = ChangingPassword; //We currently dont have a password stored so we need to create one.
        currentVaultState = AcceptingInput;

        //Write to memory the current state of the vault.
        EEPROM.write(VAULT_STATE_ADDRESS, currentVaultState);
        EEPROM.commit();

        Serial.println("Initializing Vault, Now using input to set new password.");
    } 
  
    //Check whether or not the door is currently open or closed
    if(IsDoorOpen())//Door is open
    {
        //Store when the door was opened
        timeWhenDoorUnlocked = millis();

        //Unlock the door and change the door state accordingly
        currentDoorState = Unlocked;
        Unlock();
        Serial.println("Unlocking Vault.");
    }
    else //Door is closed
    {
        //Lock the door and change the door state accordingly
        currentDoorState = Locked;
        Lock();
        timeWhenDoorLocked = millis();
        ResetInput();
        Serial.println("Locking Vault.");
    }

    //Initialize the Password & Bluetooth managers
    SetupPasswordManager(initialized);
    
    if(BLUETOOTH_ENABLED) InitializeBluetooth(&CheckInput, &SetNewPassword);
}

void loop() 
{
    //Check if the user has not been locked out as of yet.
    if(currentVaultState == AcceptingInput) //Vault has not locked the user out
    {
        //Update the Input & Bluetooth managers as they need an update loop.
        HandleInput();
        if(BLUETOOTH_ENABLED) HandleBluetooth();

        //Check if the door is currently unlocked
        if(currentDoorState == Unlocked)
        {
            //Check the current state of the physical door
            int currentPhysicalDoorState = IsDoorOpen();

            //Check if the physical door state changed
            if(currentPhysicalDoorState != previousPhysicalDoorState)
            {
                //Check if the door transitioned from open to closed
                if(!currentPhysicalDoorState) //Door was just closed
                {
                    //Save the time the door was closed on and set a flag indicating the door was closed to true
                    timeWhenDoorPhysicallyClosed = millis();
                    doorWasClosed = true;
                }
                else
                {
                    //door transitioned back to open before the door locked. Set this flag back to false.
                    doorWasClosed = false;
                }

                //Save the current state so we only trigger this once.
                previousPhysicalDoorState = currentPhysicalDoorState;
            }

            //Check if the door is closed but hasnt been opened yet. Then lock the door again after a preindicated delay.
            if(!IsDoorOpen() && (millis() - timeWhenDoorUnlocked > UNLOCK_LOCK_DELAY) && !doorWasClosed)
            {
                //Lock the door and update state & flag accordingly
                currentDoorState = Locked;
                Lock();        
                doorWasClosed = false;
                timeWhenDoorLocked = millis();
                ResetInput();
                Serial.println("Locking vault.");
            }
            else if(!IsDoorOpen() && millis() - timeWhenDoorPhysicallyClosed > LOCK_DELAY && doorWasClosed) //Check if the door was recently closed and then wait a predetermined time to lock the gate
            {
                //Lock the door and update state & flag accordingly
                currentDoorState = Locked;
                Lock();     
                doorWasClosed = false;
                timeWhenDoorLocked = millis();
                ResetInput();   
                Serial.println("Locking vault.");
            }
    
            //Scan if reset password was pressed, if so change input task to ChangingPassword 
            if(ResetPasswordButtonPressed())
            {
                //Change our input task to changing the password.
                currentInputTask = ChangingPassword;
                Serial.println("Now changing stored password with next input.");
            }
        }
        else if(IsDoorOpen() && millis() - timeWhenDoorLocked < RESET_LOCK_DELAY) //Door is open while it should be closed. User probably opened the door really quickly while the lock was engaging, causing the lock to fail.
        {
            //Record the time we unlocked the door
            timeWhenDoorUnlocked = millis();

            //Unlock the door again (If we dont we wont be abe to close the door again) and update state.
            currentDoorState = Unlocked;
            Unlock();
            Serial.println("Unlocked door as it was already open");
        }
    }
    else //Vault is currently locked down because of too many incorrect attempts to open the vault.
    {
        Serial.println("Started Lockdown timer.");

        //Set the dot to the position of the minute digit.
        _data.dotPosition = 1;

        //Loop for LOCK_TIME_SECONDS
        for(int i = LOCK_TIME_SECONDS; i > 0; i--)
        {
            //Calculate the amount of minutes/seconds remaining.
            int seconds = i % 60;
            int minutes = (i - seconds) / 60; 

            //Update the display
            _data.digits[0] = 0;
            _data.digits[1] = minutes;
            _data.digits[2] = seconds / 10 % 10;
            _data.digits[3] = seconds % 10;
            
            delay(1000);
        }

        //Reset our input (change display values back to 0, set dot position to the first digit and reset the rotary encoder's counter.
        ResetInput();
        //Flash the display to indicate the counter has finished.
        FlashDisplay(&_data);

        //Set the vault state back to accepting input and write this change to memory.
        currentVaultState = AcceptingInput;
        EEPROM.write(VAULT_STATE_ADDRESS, currentVaultState);
        EEPROM.commit();
    }
}

void ReceivedInput()
{ 
    //print the entered code. Just for debugging purposes.
    Serial.print("Entered Digits: ");
    for(int i = 0; i < DIGIT_AMOUNT; i++)
    {
        Serial.print(_data.digits[i]);
    }
    
    Serial.println();
  
    if(currentInputTask == EnteringCode) //The current input is to be handled as an attempt at opening the vault
    {
        if(currentDoorState != Unlocked) //Check if the door isnt currently unlocked
        {    
            CheckInput(_data.digits); //Handle the given input. Put into external method so bluetooth can access easily.
        }
    }
    else if (currentInputTask == ChangingPassword) //The current input is to be handled as a new password, resetting the old.
    {
        SetNewPassword(_data.digits); //Handle the given input. Put into external method so bluetooth can access easily.
    }

    ResetInput();
}

bool CheckInput(int code[DIGIT_AMOUNT])
{
    if(IsPasswordCorrect(code)) //Check if the password entered by the user is correct.
    {
        //Store when we unlocked the door. This is used for later locking the door if we dont open it.
        timeWhenDoorUnlocked = millis();
  
        //Change states of the vault and unlock the door.
        currentDoorState = Unlocked;
        Unlock();
        Serial.println("Unlocked Door");
  
        ResetInput();
        return true;
    }
    else
    {
        Serial.println("Wrong Input");
  
        //Flash the display & sound buzzer to indicate the code entered was wrong
        FlashDisplay(&_data);
        
        //Check if the user has failed to enter the correct password before. If so lock them out of putting in a code for a predefined amount of time.
        if(AmountOfCorrectTries() >= MAX_INCORRECT_TRIES)
        {
          currentVaultState = InputLocked;
          //TODO: Send bluetooth or wifi signal.
  
          //Write the current locked state to memory in case the user tries to circumvent this by unplugging power.
          EEPROM.write(VAULT_STATE_ADDRESS, currentVaultState);
          EEPROM.commit();
  
          Serial.println("Locked Input.");
        }
        SoundBuzzer();
  
        ResetInput();
        return false;
    }
}

void SetNewPassword(int code[DIGIT_AMOUNT])
{
    //Change the password we have in memory to the newly entered password.
    SetPasscode(code);
  
    //Finished changing password, change input task accordingly
    currentInputTask = EnteringCode;
    Serial.println("Set Password, now accepting codes");
}
