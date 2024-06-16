#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

char *read_rfid()
{
    FILE *fp;
    static char rfid[16];

    printf("Please scan your RFID card\n");
    fp = popen("python3 rfid_reader.py", "r");
    if (fp == NULL)
    {
        printf("Failed to load rfid_reader\n");
        exit(1);
    }

    if(fgets(rfid, sizeof(rfid) - 1, fp) != NULL)
    
        rfid[strcspn(rfid, "\n")] = '\0';
    

    pclose(fp);

    return rfid;
}

int main() {
    
    return 0;
}