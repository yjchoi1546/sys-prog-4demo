#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// 메시지 타입 정의
#define MESSAGE_TYPE_TEXT 0x01
#define MESSAGE_TYPE_FILE_REQUEST 0x02
#define MESSAGE_TYPE_FILE_RESPONSE 0x03

// 최대 메시지 크기 정의
#define MAX_MESSAGE_LENGTH 1024

// 메시지 구조체 정의
typedef struct {
    uint8_t type;          // 메시지 타입 (1바이트)
    uint16_t length;       // 메시지 길이 (2바이트)
    uint8_t body[MAX_MESSAGE_LENGTH];  // 메시지 본문 (가변 길이)
} Message;

// 함수 프로토타입
void create_message(Message *msg, uint8_t type, const uint8_t *body, uint16_t length);
void print_message(const Message *msg);

#endif // PROTOCOL_H
