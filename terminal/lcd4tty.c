#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdint.h>
#include "basic_tree.h"

// I2C 주소 및 LCD 명령 정의
#define I2C_ADDR 0x27
#define LCD_CHR 1  // 문자 모드
#define LCD_CMD 0  // 명령어 모드
#define LINE1 0x80 // 첫 번째 라인
#define LINE2 0xC0 // 두 번째 라인

// 전역 변수
int fd; // I2C 장치 파일 디스크립터

// 함수 선언
void lcd_init();
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);
void lcd_send_string(const char *str);
void display_time();

int main()
{
    // wiringPi 초기화 실패 시 오류 메시지 출력 후 종료
    if (wiringPiSetup() == -1)
    {
        fprintf(stderr, "wiringPi 초기화 실패.\n");
        exit(1);
    }

    // I2C 장치 초기화 실패 시 오류 메시지 출력 후 종료
    fd = wiringPiI2CSetup(I2C_ADDR);
    if (fd == -1)
    {
        fprintf(stderr, "I2C 초기화 실패.\n");
        exit(1);
    }

    // LCD 초기화
    lcd_init();

    // 무한 루프: 시간 표시 갱신
    while (1)
    {
        display_time("hi"); // 시간 표시 함수 호출
        delay(1000);        // 1초마다 갱신
    }

    return 0;
}

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
void display_time(char buffer[32])
{
    lcd_byte(LINE1, 4);      // 첫 번째 라인으로 커서 이동
    lcd_send_string(buffer); // 시간 문자열 전송
}