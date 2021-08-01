#include "Arduino.h"
#include "DisplayDrivers.h"

SemaphoreHandle_t displaySemaphore;

//This is an asynchronous task running on either one of the two cores of the ESP32
void RefreshDisplayTask(void * perameter)
{
    //Decode the input perameter to the correct value type.
    DisplayData *_data = (DisplayData*) perameter;
    byte code; //Reserve a variable to code our data into
    
    for(;;) //Start an infinite loop. As we are running this on other threads this isnt a problem and even desireable (Creating a second update loop)
    {
        //Check if we actually have access to the display data right now. This is done by a flag indicating if the display is flashing right now and if we can access the Semaphore for the data.
        if(!_data->displayIsFlashing && xSemaphoreTake(displaySemaphore, (TickType_t)10) == pdTRUE) 
        {
            //Loop through each digit
            for(int i = 0; i < DIGIT_AMOUNT; i++)
            {     
                //Set the dot LED to off
                digitalWrite(PIN_DOT_ON, HIGH);  
                //Check if we want to turn on the dot LED and if we are currently on the right display. If so turn on the dot LED
                if(_data->dot && i == _data->dotPosition) digitalWrite(PIN_DOT_ON, LOW);  
  
                //Write the copy pin to low, indicating to the 595 that we dont want it to read from the input register right now.
                digitalWrite(PIN_COPY, LOW);
  
                //Encode our data to an 8-bit value. (BCD[0000] + Display[0000])
                code = (15-_data->digits[i])<<4;
                code += ((0001<<i));
  
                //Shift the data out to the 595 starting at the most significant bit.
                shiftOut(PIN_WRITE_BUS, PIN_DISPLAY_CLOCK, MSBFIRST, ~code);
  
                //Write the copy pin to high, indicating to the 595 we want it to read from the input register.
                digitalWrite(PIN_COPY, HIGH);
  
                //Delay this loop for 1 millisecond. While this helps us humans seeing the digit it also stops this thread from crashing
                vTaskDelay(1 / portTICK_PERIOD_MS);
            }
            
            xSemaphoreGive(displaySemaphore); //Release our usage of the semaphore so a potential display flash can use it.
        }
        else //We cant use the semaphore anymore since the thread responsible for flashing the display wants to use it (or is already using it)
        {
            xSemaphoreGive(displaySemaphore); //Give the semaphore back in case we are using it
            vTaskDelay(1 / portTICK_PERIOD_MS); //Delay the task as we dont want it to crash. (watchdog will kill this thread when it detects the infinite loop (I think))
        }
        
    }
}

void SetupDisplayTask(DisplayData *_data)
{
  //Setup the pins we need to use to write data to the display
  pinMode(PIN_WRITE_BUS, OUTPUT);
  pinMode(PIN_DISPLAY_CLOCK, OUTPUT);
  pinMode(PIN_COPY, OUTPUT);
  pinMode(PIN_DOT_ON, OUTPUT);

  //Create a semaphore we can use during the running of our program
  displaySemaphore = xSemaphoreCreateBinary();

  //Start the display task.
  xTaskCreate(
    RefreshDisplayTask,          // Function that should be called
    "Display Refreshing",   // Name of the task (for debugging)
    1000,                 // Stack size (bytes)
    (_data),              // Parameter to pass
    3,                    // Task priority (0 - 24) 0 = lowest, 24 = highest
    NULL                  // Task handle
  );
}

void FlashDisplayTask(void *perameter)
{
    //Decode the input perameter to the correct value type.
    DisplayData *_data = (DisplayData*) perameter;
    
    for(;;) //Start an infinite loop. We dont need this for flashing our display but in case we are starting this during an update cycle of the display we need to retry. Infinite loop might not be necessary but it ensures we get a chance to flash the display
    {
        if(xSemaphoreTake(displaySemaphore, (TickType_t)10) == pdTRUE) //Check if we can take the Semaphore.
        {
            _data->displayIsFlashing = true; //Set the flashing display flag to true.
            byte code; //Reserve a byte of memory to write our display data into
            digitalWrite(PIN_DOT_ON, LOW); //turn on each dot 
            
            for(int flash = 0; flash < FLASH_AMOUNT; flash++) //Loop through each flash we need to do.
            {
                digitalWrite(PIN_COPY, LOW); //Write the copy pin to low, indicating to the 595 that we dont want it to read from the input register right now.

                //Write in the BCD value of 8 and turn each display on
                code = (15-8)<<4;
                code += 15; //1111

                //Shift the data out to the 595 starting at the most significant bit.
                shiftOut(PIN_WRITE_BUS, PIN_DISPLAY_CLOCK, MSBFIRST, ~code);

                //Write the copy pin to high, indicating to the 595 we want it to read from the input register.
                digitalWrite(PIN_COPY, HIGH);

                //Delay for half the amount of time this flash should take
                vTaskDelay((FLASH_TIME / 2) / portTICK_PERIOD_MS);
      
                digitalWrite(PIN_COPY, LOW); //Write the copy pin to low, indicating to the 595 that we dont want it to read from the input register right now.
                
                code = 0; //Set the code to 0. This means absolutely no displays will be turned on.
                
                //Shift the data out to the 595 starting at the most significant bit.
                shiftOut(PIN_WRITE_BUS, PIN_DISPLAY_CLOCK, MSBFIRST, ~code);

                //Write the copy pin to high, indicating to the 595 we want it to read from the input register.
                digitalWrite(PIN_COPY, HIGH);

                //Delay for the other half the amount of time this flash should take
                vTaskDelay((FLASH_TIME / 2) / portTICK_PERIOD_MS);
            }

            //As we have finished flashing, give the Semaphore back so the display can start refreshing itself again.
            xSemaphoreGive(displaySemaphore);

            //break out of the infinite loop.
            break;
        }
        else //If we cant delay the task by 1ms and then try to take the semaphore again.
        {
            vTaskDelay(1 / portTICK_PERIOD_MS); //Delay the task by 1ms so the watchdog doesnt stop it.
        }
    }

    //Turn of the flag indicating we are flashing
    _data->displayIsFlashing = false;
    vTaskDelete(NULL); //Delete this task.
}

void FlashDisplay(DisplayData *_data)
{
    //Create the task that will flash the display.
    xTaskCreate(
        FlashDisplayTask,     // Function that should be called
        "Flash Display",      // Name of the task (for debugging)
        1000,                 // Stack size (bytes)
        (_data),              // Parameter to pass
        4,                    // Task priority
        NULL                  // Task handle
    );
}
