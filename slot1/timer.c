#include <stdio.h>
#include <wiringPi.h>
#include <time.h>
#include <string.h>
#include <softPwm.h>
#include "lcd.h"

#define START_BUTTON_PIN 23
#define INCREASE_BUTTON_PIN 24
#define DECREASE_BUTTON_PIN 25
#define SERVO_PIN 1
#define INTERVAL 0.1

#define LCD_WIDTH 16
#define PROGRESS_BAR_WIDTH (LCD_WIDTH - 2)

void setupServo()
{
    wiringPiSetup();
    softPwmCreate(SERVO_PIN, 0, 200);
}

void rotateServo(int angle)
{
    int pulseWidth = (int)(5 + angle / 180.0 * 20);
    softPwmWrite(SERVO_PIN, pulseWidth);
    delay(1000);
}

void setup_pins()
{
    pinMode(START_BUTTON_PIN, INPUT);
    pinMode(INCREASE_BUTTON_PIN, INPUT);
    pinMode(DECREASE_BUTTON_PIN, INPUT);
    pullUpDnControl(START_BUTTON_PIN, PUD_UP);
    pullUpDnControl(INCREASE_BUTTON_PIN, PUD_UP);
    pullUpDnControl(DECREASE_BUTTON_PIN, PUD_UP);
}

void display_progress_bar(int percent)
{
    int bar_length = (percent * PROGRESS_BAR_WIDTH) / 100;
    char progress_bar[LCD_WIDTH + 1];
    for (int i = 0; i < LCD_WIDTH; i++)
    {
        if (i == 0 || i == LCD_WIDTH - 1)
        {
            progress_bar[i] = '|';
        }
        else if (i <= bar_length)
        {
            progress_bar[i] = '#';
        }
        else
        {
            progress_bar[i] = ' ';
        }
    }
    progress_bar[LCD_WIDTH] = '\0';

    lcd_clear();
    lcd_print_at(0, 1, "!rehab!");
    lcd_print_at(1, 0, progress_bar);
}

void scroll_text(const char *text)
{
    char display_text[LCD_WIDTH + 1];
    int len = strlen(text);
    int pos = 0;

    while (1)
    {
        for (int i = 0; i < LCD_WIDTH; i++)
        {
            if (i + pos < len)
            {
                display_text[i] = text[i + pos];
            }
            else
            {
                display_text[i] = ' ';
            }
        }
        display_text[LCD_WIDTH] = '\0';
        lcd_print_at(1, 0, display_text);
        delay(300);

        pos++;
        if (pos >= len)
        {
            pos = 0;
        }

        if (digitalRead(START_BUTTON_PIN) == LOW || digitalRead(INCREASE_BUTTON_PIN) == LOW || digitalRead(DECREASE_BUTTON_PIN) == LOW)
        {
            break;
        }
    }
}

int main(void)
{
    if (wiringPiSetup() == -1)
    {
        printf("Failed to setup WiringPi!\n");
        return 1;
    }

    setupServo();
    lcd_init();
    lcd_clear();
    lcd_print_at(0, 5, "!REHAB!");
    scroll_text("~~~ Give me your cell phone ^^!");

    setup_pins();
    int timer_minutes = 0;
    int timer_seconds = 0;
    int start_button, increase_button, decrease_button;
    int last_start_button = HIGH;
    int last_increase_button = HIGH;
    int last_decrease_button = HIGH;
    int start_time = 0;
    int is_timer_running = 0;

    while (1)
    {
        start_button = digitalRead(START_BUTTON_PIN);
        increase_button = digitalRead(INCREASE_BUTTON_PIN);
        decrease_button = digitalRead(DECREASE_BUTTON_PIN);

        if (increase_button == LOW && last_increase_button == HIGH)
        {
            if (!is_timer_running)
            {
                timer_minutes++;
                char buffer[16];
                snprintf(buffer, sizeof(buffer), "Time-setting: %d:%02d", timer_minutes, timer_seconds);
                lcd_clear();
                lcd_print_at(0, 0, buffer);
                printf("Timer set to %d minutes\n", timer_minutes);
            }
        }

        if (decrease_button == LOW && last_decrease_button == HIGH)
        {
            if (!is_timer_running && timer_minutes > 0)
            {
                timer_minutes--;
                char buffer[16];
                snprintf(buffer, sizeof(buffer), "Time-setting: %d:%02d", timer_minutes, timer_seconds);
                lcd_clear();
                lcd_print_at(0, 0, buffer);
                printf("Timer set to %d minutes\n", timer_minutes);
            }
        }

        if (start_button == LOW && last_start_button == HIGH)
        {
            if (!is_timer_running && timer_minutes > 0)
            {

                printf("Starting timer for %d minutes\n", timer_minutes);
                char buffer[16];
                snprintf(buffer, sizeof(buffer), "Start: %d:%02d", timer_minutes, timer_seconds);

                // Rotate servo to 0 degrees
                printf("Rotating to 0 degrees\n");
                rotateServo(90);
                delay(1000);
                lcd_clear();
                lcd_print_at(0, 0, buffer);
                start_time = (int)time(NULL);
                is_timer_running = 1;
            }
        }

        last_start_button = start_button;
        last_increase_button = increase_button;
        last_decrease_button = decrease_button;

        if (is_timer_running)
        {
            int elapsed_time = (int)time(NULL) - start_time;
            int remaining_time = (timer_minutes * 60 + timer_seconds) - elapsed_time;
            if (remaining_time <= 0)
            {
                // Rotate servo to 90 degrees
                printf("Rotating to 0 degrees\n");
                rotateServo(0);
                delay(1000);

                printf("Timer done!\n");
                lcd_clear();
                lcd_print_at(1, 1, "Timer done!");
                delay(5000);
                is_timer_running = 0;
                timer_minutes = 0;
                timer_seconds = 0;

                lcd_clear();
                lcd_print_at(0, 5, "!REHAB!");
                scroll_text("~~~ Give me your cell phone ^^!");
            }
            else
            {
                int percent_remaining = (remaining_time * 100) / (timer_minutes * 60);
                display_progress_bar(percent_remaining);
                printf("Time remaining: %d:%02d\n", remaining_time / 60, remaining_time % 60);
            }
        }
        delay(500);
    }
    return 0;
}
