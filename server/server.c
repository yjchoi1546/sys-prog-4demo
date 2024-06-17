#include "server.h"

// 전역 변수 선언
struct user users[MAX_USERS]; // 사용자 데이터 저장 배열
struct admin admins[MAX_ADMINS]; // 관리자 데이터 저장 배열
int user_count = 0;           // 현재 사용자 수
int admin_count = 0;          // 현재 관리자 수
pthread_mutex_t lock;         // 뮤텍스 락

// 사용자 데이터를 저장하는 함수
void save_user_data(char *user_id, time_t start, time_t end) {
    pthread_mutex_lock(&lock); // 데이터 접근 시 뮤텍스 잠금
    for (int i = 0; i < user_count; ++i) {
        if (strcmp(users[i].id, user_id) == 0) {
            struct interval new_interval = { start, end }; // 새로운 사용 시간 간격
            for (int j = 0; j < 10; ++j) {
                if (users[i].intervals[j].start == 0) {
                    users[i].intervals[j] = new_interval;
                    break;
                }
            }
            users[i].mileage += calculate_mileage(start, end); // 마일리지 계산 및 추가
            pthread_mutex_unlock(&lock); // 뮤텍스 잠금 해제
            return;
        }
    }
    // 새로운 사용자 추가
    struct user new_user;
    strcpy(new_user.id, user_id);
    new_user.mileage = calculate_mileage(start, end);
    new_user.intervals[0].start = start;
    new_user.intervals[0].end = end;
    users[user_count++] = new_user;
    pthread_mutex_unlock(&lock); // 뮤텍스 잠금 해제
}

// 마일리지를 계산하는 함수
int calculate_mileage(time_t start, time_t end) {
    int duration = difftime(end, start) / 60; // 사용 시간의 분 단위 계산
    return duration * 10; // 분당 10 포인트 적립
}

// 사용자 데이터를 터미널로 전송하는 함수
void send_user_data_to_terminal(int client_socket, char *user_id) {
    pthread_mutex_lock(&lock); // 데이터 접근 시 뮤텍스 잠금
    for (int i = 0; i < user_count; ++i) {
        if (strcmp(users[i].id, user_id) == 0) {
            write(client_socket, &users[i], sizeof(struct user)); // 사용자 데이터 전송
            pthread_mutex_unlock(&lock); // 뮤텍스 잠금 해제
            return;
        }
    }
    pthread_mutex_unlock(&lock); // 뮤텍스 잠금 해제
}

// 관리자 요청을 처리하는 함수
void handle_admin_request(int client_socket, char *admin_id, int request_type) {
    pthread_mutex_lock(&lock); // 데이터 접근 시 뮤텍스 잠금
    if (request_type == 1) { // 유저 리스트 조회
        for (int i = 0; i < user_count; ++i) {
            write(client_socket, &users[i], sizeof(struct user)); // 모든 사용자 데이터 전송
        }
    } else if (request_type == 2) { // 신규 관리자 계정 추가
        if (admin_count < MAX_ADMINS) {
            strcpy(admins[admin_count++].id, admin_id);
        }
    } else if (request_type == 3) { // 관리자 계정 삭제
        for (int i = 0; i < admin_count; ++i) {
            if (strcmp(admins[i].id, admin_id) == 0) {
                for (int j = i; j < admin_count - 1; ++j) {
                    admins[j] = admins[j + 1];
                }
                --admin_count;
                break;
            }
        }
    }
    pthread_mutex_unlock(&lock); // 뮤텍스 잠금 해제
}

// 클라이언트 요청을 처리하는 함수
void *client_handler(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);

    uint8_t buffer[BUFFER_SIZE]; // 버퍼 생성
    int bytes_received = read(client_socket, buffer, BUFFER_SIZE);
    if (bytes_received < 0) {
        perror("recv failed");
        close(client_socket);
        return NULL;
    }

    Message msg; // 메시지 구조체 생성
    memcpy(&msg, buffer, bytes_received); // 버퍼 데이터를 메시지 구조체로 복사

    if (msg.type == MESSAGE_TYPE_TEXT) { // 사용자 데이터 저장 메시지
        char user_id[14];
        time_t start, end;
        memcpy(user_id, msg.body, 14); // 사용자 ID 복사
        memcpy(&start, msg.body + 14, sizeof(time_t)); // 시작 시간 복사
        memcpy(&end, msg.body + 14 + sizeof(time_t), sizeof(time_t)); // 종료 시간 복사

        save_user_data(user_id, start, end); // 사용자 데이터 저장
    } else if (msg.type == MESSAGE_TYPE_FILE_REQUEST) { // 사용자 데이터 요청 메시지
        char user_id[14];
        memcpy(user_id, msg.body, 14); // 사용자 ID 복사
        send_user_data_to_terminal(client_socket, user_id); // 사용자 데이터 전송
    } else if (msg.type == MESSAGE_TYPE_FILE_RESPONSE) { // 관리자 요청 메시지
        char admin_id[14];
        int request_type;
        memcpy(admin_id, msg.body, 14); // 관리자 ID 복사
        memcpy(&request_type, msg.body + 14, sizeof(int)); // 요청 타입 복사
        handle_admin_request(client_socket, admin_id, request_type); // 관리자 요청 처리
    }

    close(client_socket); // 클라이언트 소켓 닫기
    return NULL;
}

// 서버 메인 함수
int main() {
    int server_socket, *new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    pthread_t tid;

    // 뮤텍스 초기화
    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("Mutex init failed");
        return 1;
    }

    // 소켓 생성
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 소켓 바인딩
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        return 1;
    }

    // 클라이언트 연결 대기
    if (listen(server_socket, 3) < 0) {
        perror("Listen failed");
        close(server_socket);
        return 1;
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        // 클라이언트 연결 수락
        new_socket = malloc(sizeof(int));
        *new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (*new_socket < 0) {
            perror("Accept failed");
            free(new_socket);
            continue;
        }

        // 새로운 쓰레드 생성
        if (pthread_create(&tid, NULL, client_handler, (void *)new_socket) != 0) {
            perror("Thread creation failed");
            free(new_socket);
        }

        // 쓰레드 분리
        pthread_detach(tid);
    }

    // 자원 정리
    close(server_socket);
    pthread_mutex_destroy(&lock);
    return 0;
}
