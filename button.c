#include <wiringPi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wiringPiI2C.h>

#define BUTTON1_PIN 22
#define BUTTON2_PIN 23
#define BUTTON3_PIN 24
#define BUTTON4_PIN 25

#define I2C_ADDR 0x27
#define LCD_CHR 1
#define LCD_CMD 0
#define LINE1 0x80
#define LINE2 0xC0
#define MAX_USERS 4
int numUsers = MAX_USERS;
int fd;

typedef struct
{
    char name[20];
    int mileage;
} User;

User userList[MAX_USERS] = {
    {"User 1", 10},
    {"User 2", 20},
    {"User 3", 30},
    {"User 4", 40}};

int currentUserIndex = 0;
int selectedUserIndex = -1;
int listDeleted = 0;

void lcd_init();
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);
void lcd_send_string(const char *str);
void display_user();
void display_time();
void delete_current_user();

void *buttonPressed(void *arg)
{
    int buttonPin = *((int *)arg);
    int lastState = HIGH;
    while (1)
    {
        int currentState = digitalRead(buttonPin);
        if (currentState != lastState)
        {
            if (currentState == LOW)
            {
                printf("Button on pin %d pressed\n", buttonPin);
                if (buttonPin == BUTTON1_PIN)
                {
                    currentUserIndex = (currentUserIndex + 1) % MAX_USERS;
                    display_user();
                }
                else if (buttonPin == BUTTON2_PIN)
                {
                    currentUserIndex = (currentUserIndex - 1 + MAX_USERS) % MAX_USERS;
                    display_user();
                }
                else if (buttonPin == BUTTON3_PIN)
                {
                    if (numUsers > 0)
                    {
                        delete_current_user();
                    }
                }
                else if (buttonPin == BUTTON4_PIN)
                {
                    display_time();
                }
            }
            lastState = currentState;
        }
        delay(50);
    }
    return NULL;
}

void delete_current_user()
{
    if (numUsers > 0)
    {
        for (int i = currentUserIndex; i < numUsers - 1; i++)
        {
            userList[i] = userList[i + 1];
        }
        numUsers--;
        if (currentUserIndex >= numUsers)
        {
            currentUserIndex = 0;
        }
        display_user();
    }
}

void display_time()
{
    lcd_init();
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[17];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    lcd_byte(LINE1, LCD_CMD);
    lcd_send_string(buffer);

    strftime(buffer, sizeof(buffer), "%d-%m-%Y", timeinfo);
    lcd_byte(LINE2, LCD_CMD);
    lcd_send_string(buffer);
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

void display_user()
{
    lcd_init();
    lcd_byte(LINE1, LCD_CMD);
    lcd_send_string(userList[currentUserIndex].name);
    lcd_byte(LINE2, LCD_CMD);
    if (selectedUserIndex == currentUserIndex)
    {
        lcd_send_string("Selected");
    }
    else
    {
        char userInfo[50];
        snprintf(userInfo, sizeof(userInfo), "mileage: %d", userList[currentUserIndex].mileage);
        lcd_send_string(userInfo);
    }
}

int main(void)
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

    lcd_init();
    pinMode(BUTTON1_PIN, INPUT);
    pinMode(BUTTON2_PIN, INPUT);
    pinMode(BUTTON3_PIN, INPUT);
    pinMode(BUTTON4_PIN, INPUT);

    pullUpDnControl(BUTTON1_PIN, PUD_UP);
    pullUpDnControl(BUTTON2_PIN, PUD_UP);
    pullUpDnControl(BUTTON3_PIN, PUD_UP);
    pullUpDnControl(BUTTON4_PIN, PUD_UP);

    pthread_t thread1, thread2, thread3, thread4;
    int button1 = BUTTON1_PIN;
    int button2 = BUTTON2_PIN;
    int button3 = BUTTON3_PIN;
    int button4 = BUTTON4_PIN;

    pthread_create(&thread1, NULL, buttonPressed, &button1);
    pthread_create(&thread2, NULL, buttonPressed, &button2);
    pthread_create(&thread3, NULL, buttonPressed, &button3);
    pthread_create(&thread4, NULL, buttonPressed, &button4);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);

    return 0;
}
