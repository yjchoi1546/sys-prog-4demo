#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_ADDR 0x27

#define LCD_CHR 1 // 모드 - 데이터
#define LCD_CMD 0 // 모드 - 명령어

#define LINE1 0x80 // LCD RAM 주소 줄 1
#define LINE2 0xC0 // LCD RAM 주소 줄 2

#define LCD_BACKLIGHT 0x08 // 백라이트 ON
#define ENABLE 0b00000100  // Enable 비트

void lcd_byte(int fd, int bits, int mode);
void lcd_toggle_enable(int fd, int bits);
void lcd_init(int fd);
void lcd_loc(int fd, int line);
void lcd_write(int fd, const char *s);

int main()
{
    int fd;

    if ((fd = open("/dev/i2c-1", O_RDWR)) < 0)
    {
        printf("I2C 장치를 열 수 없습니다\n");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, I2C_ADDR) < 0)
    {
        printf("I2C 장치와 연결할 수 없습니다\n");
        return 1;
    }

    lcd_init(fd); // LCD 초기화

    lcd_loc(fd, LINE1);
    lcd_write(fd, "Hello, World!");
    lcd_loc(fd, LINE2);
    lcd_write(fd, "LCD Test");

    close(fd);
    return 0;
}

void lcd_byte(int fd, int bits, int mode)
{
    int bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
    int bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;

    if (write(fd, &bits_high, 1) != 1)
    {
        printf("I2C 장치에 쓰기 오류 (high bits)\n");
    }
    lcd_toggle_enable(fd, bits_high);

    if (write(fd, &bits_low, 1) != 1)
    {
        printf("I2C 장치에 쓰기 오류 (low bits)\n");
    }
    lcd_toggle_enable(fd, bits_low);
}

void lcd_toggle_enable(int fd, int bits)
{
    usleep(500);
    bits |= ENABLE;
    if (write(fd, &bits, 1) != 1)
    {
        printf("I2C 장치에 쓰기 오류 (enable high)\n");
    }
    usleep(500);
    bits &= ~ENABLE;
    if (write(fd, &bits, 1) != 1)
    {
        printf("I2C 장치에 쓰기 오류 (enable low)\n");
    }
    usleep(500);
}

void lcd_init(int fd)
{
    lcd_byte(fd, 0x33, LCD_CMD); // 초기화
    lcd_byte(fd, 0x32, LCD_CMD); // 초기화
    lcd_byte(fd, 0x06, LCD_CMD); // 커서 이동 방향 설정
    lcd_byte(fd, 0x0C, LCD_CMD); // 디스플레이 ON, 커서 OFF, 블링크 OFF
    lcd_byte(fd, 0x28, LCD_CMD); // 데이터 길이, N = 2 라인
    lcd_byte(fd, 0x01, LCD_CMD); // 화면 지우기
    usleep(500);
}

void lcd_loc(int fd, int line)
{
    lcd_byte(fd, line, LCD_CMD);
}

void lcd_write(int fd, const char *s)
{
    if (strlen(s) > 16)
    {
        printf("Error: The string exceeds 16 characters.\n");
        return;
    }
    while (*s)
        lcd_byte(fd, *(s++), LCD_CHR);
}
