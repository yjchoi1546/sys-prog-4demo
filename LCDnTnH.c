#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdint.h>

#define I2C_ADDR 0x27
#define LCD_CHR 1
#define LCD_CMD 0
#define LINE1 0x80
#define LINE2 0xC0

#define MAX_TIMINGS 85
#define DHT_PIN 7 // GPIO pin 7

int fd;
int data[5] = {0, 0, 0, 0, 0};

void lcd_init();
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);
void lcd_send_string(const char *str);
void display_time_and_sensor();
int read_dht_data(float *temperature, float *humidity);

int main()
{
    if (wiringPiSetup() == -1)
    {
        fprintf(stderr, "Lỗi khởi tạo wiringPi.\n");
        exit(1);
    }

    fd = wiringPiI2CSetup(I2C_ADDR);
    if (fd == -1)
    {
        fprintf(stderr, "Lỗi khởi tạo I2C.\n");
        exit(1);
    }

    lcd_init(); // Khởi tạo LCD

    while (1)
    {
        display_time_and_sensor(); // Hiển thị ngày, giờ, nhiệt độ, độ ẩm
        delay(1000);               // Cập nhật mỗi 2 giây
    }

    return 0;
}

void lcd_init()
{
    lcd_byte(0x33, LCD_CMD);
    lcd_byte(0x32, LCD_CMD);
    lcd_byte(0x06, LCD_CMD);
    lcd_byte(0x0C, LCD_CMD);
    lcd_byte(0x28, LCD_CMD);
    lcd_byte(0x01, LCD_CMD);
    delayMicroseconds(500);
}

void lcd_byte(int bits, int mode)
{
    int bits_high = mode | (bits & 0xF0) | 0x08;
    int bits_low = mode | ((bits << 4) & 0xF0) | 0x08;

    wiringPiI2CWrite(fd, bits_high);
    lcd_toggle_enable(bits_high);

    wiringPiI2CWrite(fd, bits_low);
    lcd_toggle_enable(bits_low);
}

void lcd_toggle_enable(int bits)
{
    delayMicroseconds(500);
    wiringPiI2CWrite(fd, (bits | 0x04));
    delayMicroseconds(500);
    wiringPiI2CWrite(fd, (bits & ~0x04));
    delayMicroseconds(500);
}

void lcd_send_string(const char *str)
{
    while (*str)
    {
        lcd_byte(*(str++), LCD_CHR);
    }
}

void display_time_and_sensor()
{
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[32]; // Tăng kích thước buffer

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Hiển thị giờ phút giây
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    lcd_byte(LINE1, 4);
    lcd_send_string(buffer);

    // Đọc nhiệt độ và độ ẩm từ cảm biến DHT
    float temperature, humidity;
    int result = read_dht_data(&temperature, &humidity);

    if (result == 0)
    {
        // Hiển thị nhiệt độ và độ ẩm nếu đọc thành công
        snprintf(buffer, sizeof(buffer), "T: %.1fC H: %.1f%%", temperature, humidity);
        lcd_byte(LINE2, LCD_CMD);
        lcd_send_string(buffer);
    }
}

int read_dht_data(float *temperature, float *humidity)
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    pinMode(DHT_PIN, OUTPUT);
    digitalWrite(DHT_PIN, LOW);
    delay(18);
    digitalWrite(DHT_PIN, HIGH);
    delayMicroseconds(40);
    pinMode(DHT_PIN, INPUT);

    for (i = 0; i < MAX_TIMINGS; i++)
    {
        counter = 0;
        while (digitalRead(DHT_PIN) == laststate)
        {
            counter++;
            delayMicroseconds(1);
            if (counter == 255)
            {
                break;
            }
        }
        laststate = digitalRead(DHT_PIN);

        if (counter == 255)
            break;

        if ((i >= 4) && (i % 2 == 0))
        {
            data[j / 8] <<= 1;
            if (counter > 16)
                data[j / 8] |= 1;
            j++;
        }
    }

    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)))
    {
        float h = (float)((data[0] << 8) + data[1]) / 10;
        if (h > 100)
        {
            h = data[0]; // DHT11
        }
        float c = (float)((data[2] << 8) + data[3]) / 10;
        if (c > 125)
        {
            c = data[2]; // DHT11
        }
        *humidity = h;
        *temperature = c;
        return 0;
    }
    else
    {
        return -1;
    }
}
