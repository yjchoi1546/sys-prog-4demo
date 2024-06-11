#include <wiringPi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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

// 전역 변수
int fd;
int textIndex = 0;
char *texts[] = {
    "Please tag your\ncell phone",
    "Hello, master.",
    "Mileage deduction",
    "User list",
    "Mileage deduction\n-User list",
    "User list\n-Add admin account",
    "ID: 1018671476064\nMileage: 263",
    "ID: 1039200060321\nMileage: 87",
    "No more user!"};

// 함수 선언
void lcd_init();
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);
void lcd_send_string(const char *str);
void display_text();
void *buttonPressed(void *arg);
void *rfidReader(void *arg);
void initialize_button(int buttonPin);

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

// 텍스트를 LCD에 표시하는 함수
void display_text()
{
    // 현재 텍스트 가져오기
    char *text = texts[textIndex];

    // 첫 번째 줄 출력
    char line1[17] = {0};
    char line2[17] = {0};

    char *newline = strchr(text, '\n');
    if (newline)
    {
        // \n을 기준으로 문자열을 나눔
        int line1_len = newline - text;
        strncpy(line1, text, line1_len);
        line1[line1_len] = '\0';
        strncpy(line2, newline + 1, 16);
        line2[16] = '\0';
    }
    else
    {
        // 한 줄의 문자열이 16글자를 넘는 경우
        strncpy(line1, text, 16);
        line1[16] = '\0';
        if (strlen(text) > 16)
        {
            strncpy(line2, text + 16, 16);
            line2[16] = '\0';
        }
    }

    // 첫 번째 줄 출력
    lcd_byte(LINE1, LCD_CMD);
    lcd_send_string(line1);

    // 두 번째 줄 출력
    lcd_byte(LINE2, LCD_CMD);
    lcd_send_string(line2);

    textIndex = (textIndex + 1) % (sizeof(texts) / sizeof(texts[0]));
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
                display_text();
            }
            lastState = currentState;
        }
        delay(50);
    }
    return NULL;
}

// RFID 리더 스레드 함수
void *rfidReader(void *arg)
{
    while (1)
    {
        int pid = fork();
        if (pid == 0)
        {
            // 자식 프로세스에서 RFID 리더 파이썬 스크립트 실행
            execlp("python3", "python3", "rfid_reader.py", (char *)NULL);
            exit(1); // exec 호출 실패 시 종료
        }
        else if (pid > 0)
        {
            // 부모 프로세스에서 자식 프로세스 종료 대기
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            {
                // RFID 인식됨
                printf("RFID 태그 인식됨\n");
                display_text();
            }
            else
            {
                printf("RFID 태그 인식 실패\n");
            }
        }
        else
        {
            perror("fork 실패");
            exit(1);
        }
        delay(1000); // 1초 대기 후 다시 RFID 읽기
    }
    return NULL;
}

// 버튼 초기화 함수
void initialize_button(int buttonPin)
{
    pinMode(buttonPin, INPUT);
    pullUpDnControl(buttonPin, PUD_UP);
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

    // 프로그램 시작 시 첫 번째 문자열 표시
    display_text();

    // 버튼 초기화
    initialize_button(BUTTON1_PIN);
    initialize_button(BUTTON2_PIN);
    initialize_button(BUTTON3_PIN);
    initialize_button(BUTTON4_PIN);

    // 버튼 스레드 생성
    pthread_t thread1, thread2, thread3, thread4, thread5;
    int button1 = BUTTON1_PIN;
    int button2 = BUTTON2_PIN;
    int button3 = BUTTON3_PIN;
    int button4 = BUTTON4_PIN;
    pthread_create(&thread1, NULL, buttonPressed, &button1);
    pthread_create(&thread2, NULL, buttonPressed, &button2);
    pthread_create(&thread3, NULL, buttonPressed, &button3);
    pthread_create(&thread4, NULL, buttonPressed, &button4);

    // RFID 리더 스레드 생성
    pthread_create(&thread5, NULL, rfidReader, NULL);

    // 스레드 종료 대기
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    pthread_join(thread5, NULL);

    return 0;
}
