/*
  Project Name:              The Smart House
  Sub-project Name:          Password Management
  Developer Name:            Artem Tikhonov
  Development Date:          11.04.2021
  Development Time:          2:32am CEST
  Description of the Code:   This code related to The Smart House project,
                             contains the keypad code, the LCD, and system
                             approvals interactions.
*/

//***********************************************   Headers and Libraries   ***********************************************//
#include <Keypad.h>                  // For the Arduino Keypad
#include <SoftwareSerial.h>          // For combinating the LCD's transmitter and receiver into one thing in the code
                                     // The Serial LCD display from Parallax has 3 pins: GND, 5V and RX(transmitter and receiver)
SoftwareSerial ParLCD(12, 11);       // Combining the LCD's transmitter and receiver into almost one thing
                                     // in the code => ParLCD(RX,TX)

//**************************************************   Global Variables   *************************************************//
#define SensorPin A0                 // For the Soil Moisture Sensor

//*********************************************   Machine States Explonation   ********************************************//
/* The System has 4 states:

    State 1: Entering the password. State cycling until the confirmation that
             the password was entered.

    Inside the function:
    State 2: The password was entered and confirmed. The algorythm will compare
             the entered password with the setted correct password.

    State 3: The algorythm checked the entered password. Correct password - grant
             access; Incorrect - the notification is going to be send to the LCD
             display.

    State 4: The access was granted. Now, it is going to be possible to change the
             setted correct password.
*/

//**************************************************   Variables   ********************************************************//
// Privacy management:
int incorrect_password_attempts = 0; // Numnber of the incorrect password attempts

float sensorValue = 0;               // For the Soil Moisture Sensor

const String password = "1234";      // change your password here
String input_password;

// Arduino keypad's matrix (4x4):
const byte ROWS = 4;                 // Number of the rows
const byte COLS = 4;                 // Number of the columns

char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

// Keypad pins for the rows and columns of the keypad:
byte rowPins[ROWS] = {10, 9, 8, 7};  // For rows
byte colPins[COLS] = {6, 5, 4, 3};   // For columns

Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// LCD's commands (Dec => Action):
int Move = 128;                      // Position 0 line 0 character
int Empty = 12;                      // Empty buffer
int LightOn = 17;                    // Light on the LCD
int LightOff = 18;                   // Light off the LCD

//**************************************************   Functions   ********************************************************//
// State Handling
EnterCheckPassword();

//***********************************************    Arduino Setup   ******************************************************//
void setup()
{
  ParLCD.begin(9600);                // Provide serial connection with the Arduino

  input_password.reserve(32);        // Creating the maximum input characters (it is 33 here)

  ParLCD.write(Empty);               // Clear buffer from the past actions

  ParLCD.write(Move);
  ParLCD.write(LightOn);             // Turn on the display light
  ParLCD.print("      Hey!      Password please)"); // Print the greating text

  delay(6000);

  ParLCD.write(LightOff);            // Turn off the display light
  ParLCD.write(Empty);               // Cleare the buffer after the past actions
}

//************************************************   Main Function   ******************************************************//
void loop()
{

  //*************************************************   Variables   *******************************************************//
  int machine_state = 1;             // State of the machine

  bool run_program = true;           // Falg to run the program
  bool grant_access = false;         // Flag when the access should be granted

  //*********************************************   Start of the Loop   ***************************************************//
  if (incorrect_password_attempts == 3)
  {
    ParLCD.write(Move);
    ParLCD.write(LightOn);
    ParLCD.print("  3rd Attempt!  Wait 30 seconds");

    incorrect_password_attempts = 0;

    delay(30000);

    ParLCD.write(Move);
    ParLCD.print("30 Seconds have      passed     ");
    delay(3000);

    ParLCD.write(LightOff);
    ParLCD.write(Empty);
  }

  //*************************************************   1st State   *******************************************************//
  while (run_program == true)
  {
    while (machine_state == 1)
    {
      EnterCheckPassword();
    }
  }
}

//*********************************************   State One Handling   ****************************************************//
void EnterCheckPassword()
{

  char key = keypad.getKey();

  if (key)
  {
    ParLCD.print('*');

    if (key == 'D')
    {
      input_password = "";           // clear input password
      ParLCD.write(Empty);
    }
    else if (key == 'A')
    {
      if (password == input_password)
      {
        ParLCD.write(Move);
        ParLCD.write(LightOn);
        ParLCD.print("Welcome, Artem!   :)   :)   :)  ");
        ParLCD.print(sensorValue);
        delay(10000);


        for(int i = 0; i <= 100; i++){
          sensorValue = sensorValue + analogRead(SensorPin);
          delay(1);
        }
        
        sensorValue = sensorValue/100.0;
        ParLCD.print(sensorValue);
        delay(30);
        
        
        ParLCD.write(LightOff);
        ParLCD.write(Empty);
      }
      else
      {
        ParLCD.write(Move);
        ParLCD.write(LightOn);
        ParLCD.print("Incorrect");
        delay(10000);
        ParLCD.write(LightOff);
        ParLCD.write(Empty);
      }

      input_password = "";           // clear input password
    }
    else
    {
      input_password += key;         // append new character to input password string
    }
  }
}
