// servo pin 
#define PIN_SERVO 12 //21 on pcb
#define PIN_DOORSTATE 13 //22 on pcb

//variable to store the 1st servo position LOCKED
#define LOCKED 0
//variable to store the 2nd servo position UNLOCKED
#define UNLOCKED 90

//Time for the door to wait until the door can be locked again after unlock (for example if the door isnt opened)
#define UNLOCK_LOCK_DELAY 5000
#define LOCK_DELAY 2500
#define RESET_LOCK_DELAY 600

void SetupDoorDrivers();
void Lock();
void Unlock();
bool IsDoorOpen();
