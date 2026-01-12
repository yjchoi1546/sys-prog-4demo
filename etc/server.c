#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "protocol.h"

#define PORT 8080

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    unsigned char buffer[MAX_MESSAGE_LENGTH + 3] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        error("socket failed");
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        error("setsockopt");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        error("bind failed");
    }

    if (listen(server_fd, 3) < 0)
    {
        error("listen");
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        error("accept");
    }

    int valread = read(new_socket, buffer, MAX_MESSAGE_LENGTH + 3);
    if (valread < 3)
    {
        error("Received message is too short");
    }

    // 메시지 해석
    Message msg;
    msg.type = buffer[0];
    msg.length = (buffer[1] << 8) | buffer[2];
    memcpy(msg.body, &buffer[3], msg.length);

    print_message(&msg);

    // 응답 메시지 생성 및 전송
    const char *response_text = "Hello from server";
    Message response;
    create_message(&response, MESSAGE_TYPE_TEXT, (uint8_t *)response_text, strlen(response_text));

    size_t response_size = 3 + response.length; // 전체 메시지 크기 계산
    unsigned char response_buffer[response_size];
    response_buffer[0] = response.type;
    response_buffer[1] = (response.length >> 8) & 0xFF;
    response_buffer[2] = response.length & 0xFF;
    memcpy(&response_buffer[3], response.body, response.length);

    send(new_socket, response_buffer, response_size, 0);
    printf("Response message sent\n");

    close(new_socket);
    close(server_fd);
    return 0;
}
