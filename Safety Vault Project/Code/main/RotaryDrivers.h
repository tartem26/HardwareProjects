#define PIN_BUTTON_PRESS 32
#define PIN_CLOCKWISE 34
#define PIN_COUNTERCLOCKWISE 35

#define RANGE_MINIMUM_VALUE 0
#define RANGE_MAXIMUM_VALUE 9

void SetupRotaryEncoder();
void ResetRotaryCounter();
void UpdateRotaryEncoder(int *value, void (*onButtonPressCallback)());
