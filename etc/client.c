#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "protocol.h"

#define PORT 8080

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    unsigned char buffer[MAX_MESSAGE_LENGTH + 3] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        error("Socket creation error");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        error("Invalid address/ Address not supported");
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("Connection failed");
    }

    // 메시지 생성 및 전송
    const char *text = "Hello from client";
    Message msg;
    create_message(&msg, MESSAGE_TYPE_TEXT, (uint8_t *)text, strlen(text));

    size_t message_size = 3 + msg.length; // 전체 메시지 크기 계산
    unsigned char message_buffer[message_size];
    message_buffer[0] = msg.type;
    message_buffer[1] = (msg.length >> 8) & 0xFF;
    message_buffer[2] = msg.length & 0xFF;
    memcpy(&message_buffer[3], msg.body, msg.length);

    send(sock, message_buffer, message_size, 0);
    printf("Message sent\n");

    // 응답 메시지 읽기
    int valread = read(sock, buffer, MAX_MESSAGE_LENGTH + 3);
    if (valread < 3)
    {
        error("Received message is too short");
    }

    msg.type = buffer[0];
    msg.length = (buffer[1] << 8) | buffer[2];
    memcpy(msg.body, &buffer[3], msg.length);

    print_message(&msg);

    close(sock);
    return 0;
}
