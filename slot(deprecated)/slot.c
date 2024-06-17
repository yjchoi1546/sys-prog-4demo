#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <sys/select.h>
#include <fcntl.h>

#define MSG_SIZE 256

// 전역 flag
volatile char isPresent = 0;
pthread_mutex_t lock;

// 서버 정보
struct sockaddr_in serv_addr;

struct record
{
    char RFID[14];
    time_t start, end;
} record_instance;

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

char *readRFID(int pipefd)
{
    static char uid[14] = {0};
    ssize_t bytesRead = read(pipefd, uid, sizeof(uid) - 1);
    if (bytesRead == -1)
    {
        perror("읽기 오류");
        return NULL;
    }

    // NULL 종단 추가
    uid[bytesRead] = '\0';
    printf("파이썬 스크립트의 출력: %s\n", uid);

    return uid;
}

// 소켓 만들고 connect
int sendRecord(struct sockaddr_in serv_addr)
{
    int sock;
    char msg[MSG_SIZE];

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    printf("Connection established\n");

    // 메세지 타입
    write(sock, "RECORD", MSG_SIZE);

    // record 전송
    read(sock, msg, MSG_SIZE);
    if (strcmp(msg, "OK") == 0)
        write(sock, &record_instance, sizeof(struct record));

    close(sock);

    return 0;
}

// RFID태그가 인식되는지 지속적으로 점검
void *t_RFID_handler(void *arg)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("파이프 생성 오류");
        return NULL;
    }

    // 자식 프로세스 생성
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("프로세스 생성 오류");
        return NULL;
    }
    else if (pid == 0)
    { // 자식 프로세스
        // 파이프의 쓰기 파일 디스크립터를 표준 출력으로 복제
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); // 읽기 파일 디스크립터 닫기

        // 파이썬 스크립트 실행
        execlp("python3", "python3", "rfid_reader.py", NULL);
        perror("exec 오류");
        exit(EXIT_FAILURE);
    }

    close(pipefd[1]); // 쓰기 파일 디스크립터 닫기

    fd_set read_fds;
    struct timeval tv;
    int retval;

    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(pipefd[0], &read_fds);

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        retval = select(pipefd[0] + 1, &read_fds, NULL, NULL, &tv);

        if (retval == -1)
        {
            perror("select() 오류");
            break;
        }
        else if (retval)
        {
            char *RFID = readRFID(pipefd[0]);
            pthread_mutex_lock(&lock);
            if (RFID != NULL && strlen(RFID) > 0)
            {
                if (isPresent == 0)
                {
                    strncpy(record_instance.RFID, RFID, sizeof(record_instance.RFID) - 1);
                    record_instance.start = time(NULL);
                    record_instance.end = 0;
                    isPresent = 1;
                    printf("RFID detected: %s\n", RFID);
                }
            }
            else
            {
                if (isPresent == 1)
                {
                    record_instance.end = time(NULL);
                    sendRecord(serv_addr);
                    isPresent = 0;
                    printf("RFID removed\n");
                }
            }
            pthread_mutex_unlock(&lock);
        }
        else
        {
            printf("타임아웃\n");
        }
    }

    close(pipefd[0]);
    return NULL;
}

// LCD 출력
void *t_screen_handler(void *arg)
{
    time_t elapsed_time = 0;

    while (1)
    {
        pthread_mutex_lock(&lock);
        if (isPresent == 1)
        {
            elapsed_time = time(NULL) - record_instance.start;
            printf("사용 경과 시간: %ld\n", elapsed_time);
        }
        pthread_mutex_unlock(&lock);
        sleep(1);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    printf("started\n");
    if (argc != 3)
    {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    pthread_t t_screen;
    pthread_t t_RFID;

    // 뮤텍스 초기화
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        perror("Failed to initialize mutex");
        exit(1);
    }

    if (pthread_create(&t_screen, NULL, t_screen_handler, NULL) != 0)
    {
        perror("Failed to create t_screen thread");
        exit(1);
    }
    if (pthread_create(&t_RFID, NULL, t_RFID_handler, NULL) != 0)
    {
        perror("Failed to create t_RFID thread");
        exit(1);
    }

    pthread_join(t_screen, NULL);
    pthread_join(t_RFID, NULL);

    // 뮤텍스 제거
    pthread_mutex_destroy(&lock);

    return 0;
}
