#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "protocol.h" // 프로토콜 헤더 파일 포함

#define PORT 8080
#define MAX_USERS 100
#define MAX_ADMINS 10
#define BUFFER_SIZE 1024

// 사용자 사용 시간 간격을 저장하는 구조체
struct interval {
    time_t start;
    time_t end;
};

// 사용자 정보를 저장하는 구조체
struct user {
    char id[14];
    int mileage;
    struct interval intervals[10];
};

// 관리자 정보를 저장하는 구조체
struct admin {
    char id[14];
};

// 함수 프로토타입 선언
void save_user_data(char *user_id, time_t start, time_t end);
int calculate_mileage(time_t start, time_t end);
void *client_handler(void *arg);
void send_user_data_to_terminal(int client_socket, char *user_id);
void handle_admin_request(int client_socket, char *admin_id, int request_type);

#endif // SERVER_H
