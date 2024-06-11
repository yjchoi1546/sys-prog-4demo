#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lcd.h"

#define I2C_ADDR 0x27
#define LCD_CHR 1
#define LCD_CMD 0
#define LCD_BACKLIGHT 0x08

#define ENABLE 0b00000100

#define LCD_LINE1 0x80
#define LCD_LINE2 0xC0

static int lcd_fd;

void lcd_toggle_enable(int bits)
{
    usleep(500);
    wiringPiI2CReadReg8(lcd_fd, (bits | ENABLE));
    usleep(500);
    wiringPiI2CReadReg8(lcd_fd, (bits & ~ENABLE));
    usleep(500);
}

void lcd_send_byte(int bits, int mode)
{
    int bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
    int bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;

    wiringPiI2CReadReg8(lcd_fd, bits_high);
    lcd_toggle_enable(bits_high);

    wiringPiI2CReadReg8(lcd_fd, bits_low);
    lcd_toggle_enable(bits_low);
}

void lcd_clear()
{
    lcd_send_byte(0x01, LCD_CMD);
    lcd_send_byte(0x02, LCD_CMD);
}

void lcd_set_cursor(int row, int col)
{
    int row_offsets[] = {LCD_LINE1, LCD_LINE2};
    lcd_send_byte(row_offsets[row] + col, LCD_CMD);
}

void lcd_print(const char *str)
{
    while (*str)
    {
        lcd_send_byte(*(str++), LCD_CHR);
    }
}

void lcd_print_at(int row, int col, const char *str)
{
    lcd_set_cursor(row, col);
    lcd_print(str);
}

void lcd_init()
{
    if ((lcd_fd = wiringPiI2CSetup(I2C_ADDR)) == -1)
    {
        printf("Failed to init I2C communication.\n");
        exit(1);
    }

    lcd_send_byte(0x33, LCD_CMD);
    lcd_send_byte(0x32, LCD_CMD);
    lcd_send_byte(0x06, LCD_CMD);
    lcd_send_byte(0x0C, LCD_CMD);
    lcd_send_byte(0x28, LCD_CMD);
    lcd_send_byte(0x01, LCD_CMD);
    usleep(500);
}
