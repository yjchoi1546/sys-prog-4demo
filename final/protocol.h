#ifndef MESSAGE_PROTOCOL_H
#define MESSAGE_PROTOCOL_H

#include <stdint.h>

// 사용자 유형 (2비트)
#define U_NORMAL 0x00
#define U_ADMIN  0x01

// 메시지 타입 (6비트)
#define M_SAVE           0x01
#define M_UPDATE         0x02
#define M_DELETE         0x03
#define M_REQUEST_USER   0x04
#define M_REQUEST_ALL    0x05
#define M_ACK            0x06
#define M_ERROR          0x07
#define M_DATA_RESPONSE  0x08

// 메시지 구조체
typedef struct {
    uint8_t messageTypeUserType; // 메시지 타입 (상위 6비트) 및 사용자 유형 (하위 2비트)
    uint16_t dataLength;         // 데이터 길이 (2바이트)
    uint8_t *data;               // 데이터 내용 (가변 길이)
} Message;

// 메시지 생성 함수 선언
Message *createMessage(uint8_t messageType, uint8_t userType, uint16_t dataLength, uint8_t *data);
void freeMessage(Message *message);

// 메시지 처리 함수 선언
void handleSaveRequest(Message *message);
void handleUpdateRequest(Message *message);
void handleDeleteRequest(Message *message);
void handleRequestUser(Message *message);
void handleRequestAll(Message *message);
void handleAck(Message *message);
void handleError(Message *message);
void handleDataResponse(Message *message);

#endif // MESSAGE_PROTOCOL_H
