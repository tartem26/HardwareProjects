#include "BluetoothSerial.h"
#include "DisplayDrivers.h"
#include "PasswordManager.h"
#include "BluetoothHandler.h"
#include "Time.h"

BluetoothSerial ESP_BT;
bool (*ReceiveInputCallback)(int code[DIGIT_AMOUNT]);
void (*ReceiveNewPasswordCallback)(int code[DIGIT_AMOUNT]);
bool bluetoothInitialized = false;

void InitializeBluetooth(bool (*OnReceiveInputCallback)(int code[DIGIT_AMOUNT]), void (*OnReceiveNewPasswordCallback)(int code[DIGIT_AMOUNT]))
{
    //Assign callback methods to their corresponding variables.
    ReceiveInputCallback = OnReceiveInputCallback;
    ReceiveNewPasswordCallback = OnReceiveNewPasswordCallback;

    //Setup bluetooth so the user's device can pair with it.
    if(!ESP_BT.begin("UnicornVault"))
    {
        Serial.println("An error occurred initializing Bluetooth"); //Setting up bluetooth failed, show in serial monitor for debugging purposes
    }
    else
    {
        bluetoothInitialized = true; //Setting up bluetooth went fine. Set a flag to true so we know we can safely execute our bluetooth scanning methods.
        Serial.println("Bluetooth initialized");
    }
}

//Credit to Stackoverflow, I'd have done it myself but I was pressed for time and this is a beautiful solution.
//A method for splitting a string into two or more strings based on the deliminator
String SplitString(String input, char delim, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = input.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (input.charAt(i) == delim || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? input.substring(strIndex[0], strIndex[1]) : "";
}

void HandleBluetooth()
{
    if(!bluetoothInitialized) return; //Break out of handling bluetooth if we havent been able to initialize.
  
    if (ESP_BT.available()) //Check if we receive anything from Bluetooth
    { 
        //Read the received data as a string (The app only sends string data)
        String incomingMessage = ESP_BT.readString();
        Serial.print("Received:");
        Serial.println(incomingMessage);

        //Check if the first letter of the string is an E. This signifies we want to "Enter" a value to open the vault.
        if (incomingMessage[0] == 'E')
        {
            Serial.println("Entering Code");

            //Split the string on the | deliminator to get the digits by themselves.
            String codeValueString = SplitString(incomingMessage, '|', 1);
            int codeValue = codeValueString.toInt(); //Convert the string into an int.

            //Seperate the digits and write them into a temporary array.
            int code[DIGIT_AMOUNT];
            code[0] = codeValue / 1000 % 10;
            code[1] = codeValue / 100 % 10;
            code[2] = codeValue / 10 % 10;
            code[3] = codeValue  % 10;

            //Send back a 1 or 0 depending on if the code was correct.
            ESP_BT.println(ReceiveInputCallback(code));
        } 
        if (incomingMessage[0] == 'C') //The C stands for "Changing"
        {
            Serial.println("Changing Code");
            
            //Split the string on the | deliminator to get the digits by themselves.
            String codeValueString = SplitString(incomingMessage, '|', 1);
            int codeValue = codeValueString.toInt(); //Convert the string into an int.

            //Seperate the digits and write them into a temporary array.
            int code[DIGIT_AMOUNT];
            code[0] = codeValue / 1000 % 10;
            code[1] = codeValue / 100 % 10;
            code[2] = codeValue / 10 % 10;
            code[3] = codeValue  % 10;

            //Change the currently stored password to the one we just received.
            ReceiveNewPasswordCallback(code);
        } 
    }
}
