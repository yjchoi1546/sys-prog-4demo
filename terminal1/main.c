#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <cjson/cJSON.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

#define I2C_ADDR 0x27

#define LCD_CHR 1 // 모드 - 데이터
#define LCD_CMD 0 // 모드 - 명령어

#define LINE1 0x80 // LCD RAM 주소 줄 1
#define LINE2 0xC0 // LCD RAM 주소 줄 2

#define LCD_BACKLIGHT 0x08 // 백라이트 ON
#define ENABLE 0b00000100  // Enable 비트

#define BUTTON_COUNT 4
int buttonPins[BUTTON_COUNT] = {13, 19, 26, 16}; // BUTTON_PIN_1, BUTTON_PIN_2, BUTTON_PIN_3, BUTTON_PIN_4

typedef struct TreeNode
{
    char *name;
    struct TreeNode *parent;    // 부모 노드
    struct TreeNode *sibling;   // 형제 노드
    struct TreeNode **subItems; // 자식 노드 배열
    int subItemCount;           // 자식 노드 개수
} TreeNode;

int lcd_fd = -1;
TreeNode *currentNode = NULL;
int currentIndex = 0;
pthread_mutex_t lock;
pthread_cond_t cond;
int buttonEvent = 0;

int lcd_init();
void lcd_byte(int lcd_fd, int bits, int mode);
void lcd_toggle_enable(int lcd_fd, int bits);
void lcd_loc(int lcd_fd, int line);
void lcd_write(int lcd_fd, const char *s, int line);
void lcd_scroll(int lcd_fd, const char *s, int line);
void lcd_write_two_lines(int lcd_fd, const char *line1, const char *line2);
void display_menu(int lcd_fd, TreeNode *node, int index);
void handle_button_event(int lcd_fd, int button);
void *buttonThread(void *arg);
void *monitorThread(void *arg);
void *socket_thread(void *arg);
int readGPIO(int pin);
void exportGPIO(int pin);
void unexportGPIO(int pin);
void setGPIODirection(int pin, const char *direction);
void resetGPIO(int pin);
TreeNode *create_node(const char *name);
void add_sub_item(TreeNode *parent, TreeNode *child);
TreeNode *parse_json(cJSON *json);
void print_tree(TreeNode *node, int level);
void free_tree(TreeNode *node);
TreeNode *read_json(const char *filename);
void *display_thread(void *arg);
void lcd_clear(int lcd_fd);

// GPIO 설정 함수 모듈화
void setupGPIO(int pin, const char *direction);
void cleanupGPIO(int pin);

int main(int argc, char *argv[])
{
    lcd_fd = lcd_init();
    if (lcd_fd < 0)
    {
        return 1;
    }

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        setupGPIO(buttonPins[i], "high");
    }

    pthread_t displayThread;
    pthread_t buttonThreads[BUTTON_COUNT];

    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        pthread_create(&buttonThreads[i], NULL, buttonThread, &buttonPins[i]);
    }

    pthread_create(&displayThread, NULL, display_thread, NULL);

    const char *filename = "menu.json";
    TreeNode *root = read_json(filename);

    // 초기 메뉴 설정
    currentNode = root;
    currentIndex = 0;
    display_menu(lcd_fd, currentNode, currentIndex);

    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        pthread_join(buttonThreads[i], NULL);
    }

    pthread_join(displayThread, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    close(lcd_fd);

    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        cleanupGPIO(buttonPins[i]);
    }

    free_tree(root);

    return 0;
}

void setupGPIO(int pin, const char *direction)
{
    exportGPIO(pin);
    setGPIODirection(pin, direction);
}

void cleanupGPIO(int pin)
{
    unexportGPIO(pin);
}

void exportGPIO(int pin)
{
    char buffer[4];
    int len = snprintf(buffer, sizeof(buffer), "%d", pin);
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0)
    {
        perror("Failed to open export for writing");
        exit(1);
    }
    write(fd, buffer, len);
    close(fd);
}

void unexportGPIO(int pin)
{
    char buffer[4];
    int len = snprintf(buffer, sizeof(buffer), "%d", pin);
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0)
    {
        perror("Failed to open unexport for writing");
        exit(1);
    }
    if (write(fd, buffer, len) < 0)
    {
        printf("bcm: %d\n", pin);
        perror("Failed to unexport pin");
        close(fd);
        exit(1);
    }
    close(fd);
}

void setGPIODirection(int pin, const char *direction)
{
    char path[35];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    int fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        perror("Failed to open gpio direction for writing");
        exit(1);
    }
    if (write(fd, direction, strlen(direction)) < 0)
    {
        perror("Failed to set direction");
        close(fd);
        exit(1);
    }
    close(fd);
}

int readGPIO(int pin)
{
    char path[30];
    char value_str[3];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("Failed to open gpio value for reading");
        exit(1);
    }
    if (read(fd, value_str, 3) < 0)
    {
        perror("Failed to read value");
        close(fd);
        exit(1);
    }
    close(fd);
    return atoi(value_str);
}

void resetGPIO(int pin)
{
    unexportGPIO(pin);
    exportGPIO(pin);
}

void *buttonThread(void *arg)
{
    int pin = *(int *)arg;
    int previousState = 1;
    int button;

    if (pin == buttonPins[0])
        button = 1; // 위 버튼
    else if (pin == buttonPins[1])
        button = 2; // 왼쪽 버튼
    else if (pin == buttonPins[2])
        button = 3; // 아래 버튼
    else if (pin == buttonPins[3])
        button = 4; // 오른쪽 버튼

    while (1)
    {
        int buttonState = readGPIO(pin);
        if (buttonState == 0 && previousState == 1)
        {
            handle_button_event(lcd_fd, button);
        }
        previousState = buttonState;
        usleep(50000); // 0.05초 대기
    }
    return NULL;
}

TreeNode *create_node(const char *name)
{
    TreeNode *node = malloc(sizeof(TreeNode));
    node->name = strdup(name);
    node->parent = NULL;
    node->sibling = NULL;
    node->subItems = NULL;
    node->subItemCount = 0;
    return node;
}

void add_sub_item(TreeNode *parent, TreeNode *child)
{
    if (parent->subItemCount > 0)
    {
        parent->subItems[parent->subItemCount - 1]->sibling = child;
    }

    parent->subItems = realloc(parent->subItems, sizeof(TreeNode *) * (parent->subItemCount + 1));
    parent->subItems[parent->subItemCount] = child;
    parent->subItemCount++;
    child->parent = parent;
}

TreeNode *parse_json(cJSON *json)
{
    const cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
    TreeNode *node = create_node(cJSON_GetStringValue(name));

    const cJSON *subItems = cJSON_GetObjectItemCaseSensitive(json, "subItems");
    if (cJSON_IsArray(subItems))
    {
        cJSON *subItem = NULL;
        cJSON_ArrayForEach(subItem, subItems)
        {
            TreeNode *child = parse_json(subItem);
            add_sub_item(node, child);
        }
    }

    return node;
}

void print_tree(TreeNode *node, int level)
{
    for (int i = 0; i < level; i++)
    {
        printf("  ");
    }
    printf("%s\n", node->name);

    for (int i = 0; i < node->subItemCount; i++)
    {
        print_tree(node->subItems[i], level + 1);
    }
}

void free_tree(TreeNode *node)
{
    for (int i = 0; i < node->subItemCount; i++)
    {
        free_tree(node->subItems[i]);
    }
    free(node->name);
    free(node->subItems);
    free(node);
}

TreeNode *read_json(const char *filename)
{
    FILE *fp;
    char buffer[4096];

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Unable to open file");
        return NULL;
    }

    fread(buffer, sizeof(char), sizeof(buffer), fp);
    fclose(fp);

    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL)
    {
        fprintf(stderr, "Error parsing JSON\n");
        return NULL;
    }

    TreeNode *root = parse_json(json);
    print_tree(root, 10);
    cJSON_Delete(json);

    return root;
}

void lcd_byte(int lcd_fd, int bits, int mode)
{
    int bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
    int bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;

    if (write(lcd_fd, &bits_high, 1) != 1)
    {
        printf("I2C 장치에 쓰기 오류 (high bits)\n");
    }
    lcd_toggle_enable(lcd_fd, bits_high);

    if (write(lcd_fd, &bits_low, 1) != 1)
    {
        printf("I2C 장치에 쓰기 오류 (low bits)\n");
    }
    lcd_toggle_enable(lcd_fd, bits_low);
}

void lcd_toggle_enable(int lcd_fd, int bits)
{
    usleep(500);
    bits |= ENABLE;
    if (write(lcd_fd, &bits, 1) != 1)
    {
        printf("I2C 장치에 쓰기 오류 (enable high)\n");
    }
    usleep(500);
    bits &= ~ENABLE;
    if (write(lcd_fd, &bits, 1) != 1)
    {
        printf("I2C 장치에 쓰기 오류 (enable low)\n");
    }
}

int lcd_init()
{
    int lcd_fd;

    if ((lcd_fd = open("/dev/i2c-1", O_RDWR)) < 0)
    {
        printf("I2C 장치를 열 수 없습니다\n");
        return -1;
    }

    if (ioctl(lcd_fd, I2C_SLAVE, I2C_ADDR) < 0)
    {
        printf("I2C 장치와 연결할 수 없습니다\n");
        close(lcd_fd);
        return -1;
    }

    lcd_byte(lcd_fd, 0x33, LCD_CMD); // 초기화
    lcd_byte(lcd_fd, 0x32, LCD_CMD); // 초기화
    lcd_byte(lcd_fd, 0x06, LCD_CMD); // 커서 이동 방향 설정
    lcd_byte(lcd_fd, 0x0C, LCD_CMD); // 디스플레이 ON, 커서 OFF, 블링크 OFF
    lcd_byte(lcd_fd, 0x28, LCD_CMD); // 데이터 길이, N = 2 라인
    lcd_byte(lcd_fd, 0x01, LCD_CMD); // 화면 지우기
    usleep(500);

    return lcd_fd;
}

void lcd_write(int lcd_fd, const char *s, int line)
{
    lcd_byte(lcd_fd, line, LCD_CMD);
    if (strlen(s) > 16)
    {
        lcd_scroll(lcd_fd, s, line);
    }
    else
    {
        while (*s)
            lcd_byte(lcd_fd, *(s++), LCD_CHR);
    }
}

void lcd_scroll(int lcd_fd, const char *s, int line)
{
    size_t len = strlen(s);
    char display[17];

    while (1)
    {
        for (size_t i = 0; i <= len - 16; i++)
        {
            strncpy(display, s + i, 16);
            display[16] = '\0';
            lcd_write(lcd_fd, display, line);
            usleep(500000); // 0.5초 대기
        }

        for (ssize_t i = len - 16; i >= 0; i--)
        {
            strncpy(display, s + i, 16);
            display[16] = '\0';
            lcd_write(lcd_fd, display, line);
            usleep(500000); // 0.5초 대기
        }
    }
}

void handle_button_event(int lcd_fd, int button)
{
    if (button == 1) // 위 버튼
    {
        currentIndex = (currentIndex - 1 + currentNode->subItemCount) % currentNode->subItemCount;
    }
    else if (button == 3) // 아래 버튼
    {
        currentIndex = (currentIndex + 1) % currentNode->subItemCount;
    }
    else if (button == 4) // 오른쪽 버튼 (하위 메뉴로 이동)
    {
        if (currentNode->subItemCount > 0)
        {
            currentNode = currentNode->subItems[currentIndex];
            currentIndex = 0;
        }
    }
    else if (button == 2) // 왼쪽 버튼 (상위 메뉴로 이동)
    {
        if (currentNode->parent != NULL)
        {
            currentNode = currentNode->parent;
            currentIndex = 0;
        }
    }
    lcd_clear(lcd_fd);
    display_menu(lcd_fd, currentNode, currentIndex);
}

void lcd_clear(int lcd_fd)
{
    lcd_byte(lcd_fd, 0x01, LCD_CMD); // Clear display command
    usleep(2000);                    // Wait for the command to complete
}

void display_menu(int lcd_fd, TreeNode *node, int index)
{
    if (node == NULL || node->subItemCount == 0)
        return;

    TreeNode *current = node->subItems[index];
    TreeNode *next = current->sibling ? current->sibling : node->subItems[0];

    char display_line1[17];
    char display_line2[17];

    snprintf(display_line1, sizeof(display_line1), "> %s", current->name);
    snprintf(display_line2, sizeof(display_line2), "  %s", next->name);

    // 첫 번째 줄에 현재 항목 표시
    lcd_write(lcd_fd, display_line1, LINE1);

    // 두 번째 줄에 다음 항목 표시
    lcd_write(lcd_fd, display_line2, LINE2);
}

void *display_thread(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&lock);
        while (buttonEvent == 0)
        {
            pthread_cond_wait(&cond, &lock);
        }

        handle_button_event(lcd_fd, buttonEvent);
        buttonEvent = 0;

        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void *socket_thread(void *arg)
{
    int sockfd;
    struct sockaddr_in servaddr;

    // 소켓 생성
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // 서버 정보 설정
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        pthread_exit(NULL);
    }

    // 서버에 연결
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Connection failed");
        close(sockfd);
        pthread_exit(NULL);
    }

    char *message = "Hello from client";
    send(sockfd, message, strlen(message), 0);
    printf("Message sent to server: %s\n", message);

    char buffer[1024] = {0};
    int bytesRead = read(sockfd, buffer, sizeof(buffer));
    if (bytesRead > 0)
    {
        printf("Message from server: %s\n", buffer);
    }

    close(sockfd);
    pthread_exit(NULL);
}
