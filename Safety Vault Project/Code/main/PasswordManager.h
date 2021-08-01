#define PIN_RESET_PASSWORD_BUTTON 2 //23 on pcb
#define PIN_BUZZER 15 //13 on pcb

#define STORED_PASSCODE_ADDRESS 1
#define INCORRECT_TRIES_ADDRESS 1 + sizeof(int[4]) //Calculate size of the memory block we use to store our code.

#define MAX_INCORRECT_TRIES 3
#define LOCK_TIME_SECONDS 180
#define BUZZER_SOUND_TIME 1000

void SetupPasswordManager(bool alreadyInitialized);
bool ResetPasswordButtonPressed();

bool IsPasswordCorrect(int code[4]);
int AmountOfCorrectTries();
void SetPasscode(int code[4]);
void SoundBuzzer();
