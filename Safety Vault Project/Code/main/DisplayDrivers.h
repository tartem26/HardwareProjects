#define DIGIT_AMOUNT 4
#define FLASH_AMOUNT 3
#define FLASH_TIME 900

#define PIN_WRITE_BUS 16
#define PIN_DISPLAY_CLOCK 17
#define PIN_COPY 18
#define PIN_DOT_ON 19

//This struct holds all the data we need for our threads to display the values we want
struct DisplayData
{
  int digits[DIGIT_AMOUNT] = {0,0,0,0};
  
  bool dot = true;
  int dotPosition;

  bool displayIsFlashing = false;
};

void SetupDisplayTask(DisplayData *_data);
void FlashDisplay(DisplayData *_data);
