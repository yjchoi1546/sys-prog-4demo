#include "protocol.h"
#include <stdio.h>
#include <string.h>

// 메시지 생성 함수
void create_message(Message *msg, uint8_t type, const uint8_t *body, uint16_t length)
{
    msg->type = type;
    msg->length = length;
    memcpy(msg->body, body, length);
}

// 메시지 출력 함수
void print_message(const Message *msg)
{
    printf("Message Type: %d\n", msg->type);
    printf("Message Length: %d\n", msg->length);
    printf("Message Body: %.*s\n", msg->length, msg->body);
}
