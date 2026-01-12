#include "protocol.h"
#include <stdlib.h>
#include <string.h>

// 메시지 생성 함수 구현
Message *createMessage(uint8_t messageType, uint8_t userType, uint16_t dataLength, uint8_t *data) {
    Message *message = (Message *)malloc(sizeof(Message));
    if (message == NULL) {
        return NULL; // 메모리 할당 실패
    }

    message->messageTypeUserType = (messageType << 2) | (userType & 0x03);
    message->dataLength = dataLength;
    message->data = (uint8_t *)malloc(dataLength);
    if (message->data == NULL) {
        free(message);
        return NULL; // 메모리 할당 실패
    }

    memcpy(message->data, data, dataLength);
    return message;
}

void freeMessage(Message *message) {
    if (message != NULL) {
        if (message->data != NULL) {
            free(message->data);
        }
        free(message);
    }
}

// 메시지 처리 함수 구현
void handleSaveRequest(Message *message) {
    // Save 요청 처리 로직
}

void handleUpdateRequest(Message *message) {
    // Update 요청 처리 로직
}

void handleDeleteRequest(Message *message) {
    // Delete 요청 처리 로직
}

void handleRequestUser(Message *message) {
    // Request User 요청 처리 로직
}

void handleRequestAll(Message *message) {
    // Request All 요청 처리 로직
}

void handleAck(Message *message) {
    // ACK 처리 로직
}

void handleError(Message *message) {
    // Error 처리 로직
}

void handleDataResponse(Message *message) {
    // Data Response 처리 로직
}
