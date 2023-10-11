#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "hardware/resets.h"

#define SCK 10
#define MOSI 11
#define MISO 16 // not used

#define CS1 2
#define CS2 1
#define CS3 13

#define SPI_PORT spi1

#define USBCOMMSBUFFER_LENGTH 256

#define __INT16_MIN__ -32768

//// ADC Currentsense PID controller
//// freeRTOS einbinden: usb read
////                     currentsource
////                     tempCheck


int selectedDAC = 0;
int setVoltageDAC = 0;

char usbCommsBuffer[USBCOMMSBUFFER_LENGTH];
bool hasNewData = false;

void dacSet(int setVal, int cs)
{
    printf("IO%d set to %d", cs, setVal); //Debug mode
    if (cs != 0)
    {
        uint8_t send[3];
        send[0] = 0b0;
        send[1] = setVal >> 8;
        send[2] = setVal & 0xFF;

        gpio_put(cs, 0);
        spi_write_blocking(SPI_PORT, send, 3);
        gpio_put(cs, 1);
    }
}

void shiftLeft(char *string, int shiftLength)
{
    int i, size = strlen(string);
    if (shiftLength >= size)
    {
        memset(string, '\0', size);
        return;
    }

    for (i = 0; i < size - shiftLength; i++)
    {
        string[i] = string[i + shiftLength];
        string[i + shiftLength] = '\0';
    }
}

void selectingDAC(int selectInt)
{
    switch (selectInt)
    {
    case -1:
        reset_usb_boot(0, 0); // ignores following voltage
        
        break;
    case 1:
        selectedDAC = CS1;
        break;
    case 2:
        selectedDAC = CS2;
        break;
    case 3:
        selectedDAC = CS3;
        break;

    default:
        selectedDAC = 0;
        break;
    }
}

void selectingVoltage(int selectVoltage)
{

    if (__INT16_MIN__+1 <= selectVoltage <= 32766)

    {
        dacSet(selectVoltage, selectedDAC); // set Voltage
        selectedDAC = 0;
        setVoltageDAC = 0; // restet selected
    }
    else
    {
        printf("Voltage out of 16 bit (signed) range");
    }
}
// 1,40000000
void usbRead()
{
    int usbCommsBufferindex = 0;
    while (true)
    {
        int c = getchar_timeout_us(100);
        if (c != PICO_ERROR_TIMEOUT && usbCommsBufferindex < USBCOMMSBUFFER_LENGTH)
        {
            hasNewData = true;
            usbCommsBuffer[usbCommsBufferindex++] = (c & 0xFF);
        }
        else if (hasNewData)
        {
            usbCommsBuffer[usbCommsBufferindex++] = ('\0' & 0xFF);
            int x = atoi(usbCommsBuffer);

            selectingDAC(x);

            shiftLeft(usbCommsBuffer, 2);

            x = atoi(usbCommsBuffer);

            selectingVoltage(x);
            hasNewData = false;
            usbCommsBufferindex = 0;
        }
        sleep_ms(10);
    }
}

int main()
{
    stdio_init_all();
    

    spi_init(spi1, 100 * 1000);
    spi_set_format(spi1, // SPI instance
                   8,    // Number of bits per transfer
                   0,    // Polarity (CPOL)
                   1,    // Phase (CPHA)
                   SPI_MSB_FIRST);

    gpio_set_function(SCK, GPIO_FUNC_SPI);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);
    gpio_set_function(MISO, GPIO_FUNC_SPI); // NOT USED

    gpio_init(CS1);
    gpio_set_dir(CS1, GPIO_OUT);
    gpio_put(CS1, 1); // Disable CS on startup

    gpio_init(CS2);
    gpio_set_dir(CS2, GPIO_OUT);
    gpio_put(CS2, 1); // Disable CS on startup

    gpio_init(CS3);
    gpio_set_dir(CS3, GPIO_OUT);
    gpio_put(CS3, 1); // Disable CS on startup
    int16_t b = 0;
    for (int i = 0; i < 200; i++)
    {
        dacSet(-5000+b,2); //1,-30000      -1    
        b+=50;
        sleep_ms(100);
    }
    dacSet(0,2);
    while (true)
    {
        usbRead();
    }
}