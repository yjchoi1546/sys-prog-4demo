#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pigpio.h>
#include <pthread.h>
#include <stdint.h>

// LCD
#define I2C_ADDR 0x27
#define LCD_CHR  1
#define LCD_CMD  0
#define LINE1    0x80
#define LINE2    0xC0

//  DHT
#define MAX_TIMINGS 85
#define DHT_PIN 4 // GPIO pin 4
int fd;
int data[5] = {0, 0, 0, 0, 0};

void lcd_init();
void lcd_byte(int bits, int mode);
void lcd_toggle_enable(int bits);
void lcd_send_string(const char *str);
void *display_time(void *arg);
void *read_dht(void *arg);
int read_dht_data(float *temperature, float *humidity);

int main() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Lỗi khởi tạo pigpio.\n");
        exit(1);
    }

    fd = i2cOpen(1, I2C_ADDR, 0);
    if (fd < 0) {
        fprintf(stderr, "Lỗi khởi tạo I2C.\n");
        exit(1);
    }

    lcd_init(); // init LCD

    pthread_t time_thread, dht_thread;

    // init thread timer
    if (pthread_create(&time_thread, NULL, display_time, NULL) != 0) {
        fprintf(stderr, "Error to init time thread.\n");
        exit(1);
    }

    // init thread dht
    if (pthread_create(&dht_thread, NULL, read_dht, NULL) != 0) {
        fprintf(stderr, "Error to init DHT thread.\n");
        exit(1);
    }

    pthread_join(time_thread, NULL);
    pthread_join(dht_thread, NULL);

    i2cClose(fd);
    gpioTerminate();

    return 0;
}

void lcd_init() {
    lcd_byte(0x33, LCD_CMD);
    lcd_byte(0x32, LCD_CMD);
    lcd_byte(0x06, LCD_CMD);
    lcd_byte(0x0C, LCD_CMD);
    lcd_byte(0x28, LCD_CMD);
    lcd_byte(0x01, LCD_CMD);
    gpioDelay(500);
}

void lcd_byte(int bits, int mode) {
    int bits_high = mode | (bits & 0xF0) | 0x08;
    int bits_low = mode | ((bits << 4) & 0xF0) | 0x08;

    i2cWriteByte(fd, bits_high);
    lcd_toggle_enable(bits_high);

    i2cWriteByte(fd, bits_low);
    lcd_toggle_enable(bits_low);
}

void lcd_toggle_enable(int bits) {
    gpioDelay(500);
    i2cWriteByte(fd, (bits | 0x04));
    gpioDelay(500);
    i2cWriteByte(fd, (bits & ~0x04));
    gpioDelay(500);
}

void lcd_send_string(const char *str) {
    while (*str) {
        lcd_byte(*(str++), LCD_CHR);
    }
}

void *display_time(void *arg) {
    char buffer[32]; //up size buffer
    while (1) {
        time_t rawtime;
        struct tm *timeinfo;

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        // show up time(hour,minute,second)
        strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
        lcd_byte(LINE1, LCD_CMD);
        lcd_send_string(buffer);

        gpioDelay(1000000); 
    }
    return NULL;
}

void *read_dht(void *arg) {
    char buffer[32];
    while (1) {
        // Read temperature and humidity from DHT sensor
        float temperature, humidity;
        int result = read_dht_data(&temperature, &humidity);

        if (result == 0) {
            // Display temperature and humidity if read successfully
            snprintf(buffer, sizeof(buffer), "T: %.1fC H: %.1f%%", temperature, humidity);
            lcd_byte(LINE2, LCD_CMD);
            lcd_send_string(buffer);
        }

        gpioDelay(2000000); // Update every 2 seconds
    }
    return NULL;
}

int read_dht_data(float *temperature, float *humidity) {
    uint8_t laststate = PI_HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    // Pull ping low for 18ms
    gpioSetMode(DHT_PIN, PI_OUTPUT);
    gpioWrite(DHT_PIN, PI_LOW);
    gpioDelay(18000);
    // Then pull the ping high for 40us
    gpioWrite(DHT_PIN, PI_HIGH);
    gpioDelay(40);
    // Change pin mode to INPUT
    gpioSetMode(DHT_PIN, PI_INPUT);

    // Read data from DHT sensors
    for (i = 0; i < MAX_TIMINGS; i++) {
        counter = 0;
        while (gpioRead(DHT_PIN) == laststate) {
            counter++;
            gpioDelay(1);
            if (counter == 255) {
                break;
            }
        }
        laststate = gpioRead(DHT_PIN);

        if (counter == 255) break;

        
        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<= 1;
            if (counter > 16) data[j / 8] |= 1;
            j++;
        }
    }

    
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        float h = (float)((data[0] << 8) + data[1]) / 10;
        if (h > 100) {
            h = data[0]; // DHT11
        }
        float c = (float)((data[2] << 8) + data[3]) / 10;
        if (c > 125) {
            c = data[2]; // DHT11
        }
        *humidity = h;
        *temperature = c;
        return 0; 
    } else {
        return -1; 
    }
}

