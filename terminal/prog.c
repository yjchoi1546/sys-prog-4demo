#include <wiringPi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wiringPiI2C.h>

// I2C 주소 및 LCD 명령 정의
#define I2C_ADDR 0x27
#define LCD_CHR 1  // 문자 모드
#define LCD_CMD 0  // 명령어 모드
#define LINE1 0x80 // 첫 번째 라인
#define LINE2 0xC0 // 두 번째 라인

// 버튼 핀 정의
#define BUTTON1_PIN 27
#define BUTTON2_PIN 23
#define BUTTON3_PIN 24
#define BUTTON4_PIN 25

// 최대 사용자 수 및 사용자 정보 구조체 정의
#define MAX_USERS 4
typedef struct
{
    char name[20];
    int mileage;
} User;

// 전역 변수
int numUsers = MAX_USERS;
int fd;
User userList[MAX_USERS] = {
    {"User 1", 10},
    {"User 2", 20},
    {"User 3", 30},
    {"User 4", 40}};
int currentUserIndex = 0;
int selectedUserIndex = -1;
int listDeleted = 0;

// 함수 선언
void lcd_init();
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);
void lcd_send_string(const char *str);
void display_time();
void display_user();
void delete_current_user();
void *buttonPressed(void *arg);

// LCD 초기화 함수
void lcd_init()
{
    lcd_byte(0x33, LCD_CMD); // 초기화 명령어
    lcd_byte(0x32, LCD_CMD); // 초기화 명령어
    lcd_byte(0x06, LCD_CMD); // 커서 이동 설정
    lcd_byte(0x0C, LCD_CMD); // 디스플레이 온, 커서 오프
    lcd_byte(0x28, LCD_CMD); // 4비트 모드, 2라인, 5x7 매트릭스
    lcd_byte(0x01, LCD_CMD); // 디스플레이 클리어
    delayMicroseconds(500);  // 명령 처리 시간 대기
}

// 명령어/데이터를 LCD로 전송하는 함수
void lcd_byte(int bits, int mode)
{
    int bits_high = mode | (bits & 0xF0) | 0x08;       // 상위 4비트 전송
    int bits_low = mode | ((bits << 4) & 0xF0) | 0x08; // 하위 4비트 전송

    wiringPiI2CWrite(fd, bits_high); // 상위 4비트 쓰기
    lcd_toggle_enable(bits_high);    // Enable 신호

    wiringPiI2CWrite(fd, bits_low); // 하위 4비트 쓰기
    lcd_toggle_enable(bits_low);    // Enable 신호
}

// Enable 신호 토글 함수
void lcd_toggle_enable(int bits)
{
    delayMicroseconds(500);               // 대기
    wiringPiI2CWrite(fd, (bits | 0x04));  // Enable high
    delayMicroseconds(500);               // 대기
    wiringPiI2CWrite(fd, (bits & ~0x04)); // Enable low
    delayMicroseconds(500);               // 대기
}

// 문자열을 LCD로 전송하는 함수
void lcd_send_string(const char *str)
{
    while (*str) // 문자열 끝까지 반복
    {
        lcd_byte(*(str++), LCD_CHR); // 각 문자 전송
    }
}

// 시간 정보를 LCD에 표시하는 함수
void display_time()
{
    lcd_byte(LINE1, LCD_CMD); // 첫 번째 라인으로 커서 이동
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[17];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    lcd_send_string(buffer); // 시간 문자열 전송

    lcd_byte(LINE2, LCD_CMD); // 두 번째 라인으로 커서 이동
    strftime(buffer, sizeof(buffer), "%d-%m-%Y", timeinfo);
    lcd_send_string(buffer); // 날짜 문자열 전송
}

// 사용자 정보를 LCD에 표시하는 함수
void display_user()
{
    lcd_byte(LINE1, LCD_CMD);                         // 첫 번째 라인으로 커서 이동
    lcd_send_string(userList[currentUserIndex].name); // 사용자 이름 전송

    lcd_byte(LINE2, LCD_CMD); // 두 번째 라인으로 커서 이동
    char userInfo[50];
    if (selectedUserIndex == currentUserIndex)
    {
        lcd_send_string("Selected");
    }
    else
    {
        snprintf(userInfo, sizeof(userInfo), "mileage: %d", userList[currentUserIndex].mileage);
        lcd_send_string(userInfo); // 사용자 마일리지 정보 전송
    }
}

// 현재 사용자 삭제 함수
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

// 버튼 입력 스레드 함수
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

int main(void)
{
    if (wiringPiSetup() == -1)
    {
        fprintf(stderr, "wiringPi 초기화 실패.\n");
        exit(1);
    }

    // I2C 장치 초기화
    fd = wiringPiI2CSetup(I2C_ADDR);
    if (fd == -1)
    {
        fprintf(stderr, "I2C 초기화 실패.\n");
        exit(1);
    }

    // LCD 초기화
    lcd_init();

    // 버튼 초기화
    pinMode(BUTTON1_PIN, INPUT);
    pinMode(BUTTON2_PIN, INPUT);
    pinMode(BUTTON3_PIN, INPUT);
    pinMode(BUTTON4_PIN, INPUT);
    pullUpDnControl(BUTTON1_PIN, PUD_UP);
    pullUpDnControl(BUTTON2_PIN, PUD_UP);
    pullUpDnControl(BUTTON3_PIN, PUD_UP);
    pullUpDnControl(BUTTON4_PIN, PUD_UP);

    // 버튼 스레드 생성
    pthread_t thread1, thread2, thread3, thread4;
    int button1 = BUTTON1_PIN;
    int button2 = BUTTON2_PIN;
    int button3 = BUTTON3_PIN;
    int button4 = BUTTON4_PIN;
    pthread_create(&thread1, NULL, buttonPressed, &button1);
    pthread_create(&thread2, NULL, buttonPressed, &button2);
    pthread_create(&thread3, NULL, buttonPressed, &button3);
    pthread_create(&thread4, NULL, buttonPressed, &button4);

    // 스레드 종료 대기
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);

    return 0;
}