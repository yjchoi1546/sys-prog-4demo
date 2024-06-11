#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MSG_SIZE 256

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void *client_handler()
{
    printf("수신 완료\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int serv_sock;
    struct sockaddr_in serv_addr;
    pthread_t t_client;

    // 서버 소켓 생성
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    // 서버 주소 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    // 서버 소켓에 주소 할당
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    // 클라이언트 연결 대기
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        int clnt_sock;
        struct sockaddr_in clnt_addr;
        socklen_t clnt_addr_size = sizeof(clnt_addr);

        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1)
            continue; // 클라이언트 연결 실패 시 다음 클라이언트 대기

        // 새로운 클라이언트마다 스레드 생성
        pthread_create(&t_client, NULL, client_handler, (void *)&clnt_sock);
        pthread_detach(t_client); // 스레드 종료 시 자원 반환
    }

    close(serv_sock);

    return 0;
}
