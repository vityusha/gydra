#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#ifdef WIN32
#define DEBUG_FILE_NAME  "d:\\remle.raw"
#else
#define DEBUG_FILE_NAME  "audio/remle.raw"
#endif

#include "net.h"
#include "params.h"
#include "protocol.h"

#define DEBUG_FILE       1
#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef WIN32
#define INITSOCKET(s) \
    WSADATA wsaData; \
    WSAStartup(MAKEWORD(1, 1), &wsaData); \
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
#define INITSOCKET(s)     s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif

#ifdef WIN32
#define CLOSESOCKET(s) closesocket(s); WSACleanup();
#else
#define CLOSESOCKET(s) close(s);
#endif

/***********************************************************************************/

int check_timeout(gydra_param_t* params)
{
    return (params->nsec != -1 && time(NULL) - params->starttime > params->nsec);
}

void make_cmd(unsigned char* data, unsigned char cmd, int npack)
{
    data[0] = cmd;
    data[1] = (unsigned char)((npack >> 8) & 0xFF);
    data[2] = (unsigned char)((npack >> 0) & 0xFF);
}

/***********************************************************************************/

int getdata_thread(gydra_param_t* params)
{
    void *data = NULL;

    int bytes        = 0;
    int recv_packets = 0;

    FILE* wfile = NULL;
    if(params->writefile) {
        wfile = fopen(params->file, "wb");
        if(wfile == NULL) {
            printf("Could not open output file. Exiting!\n");
            return -1;
        }
    }

#if DEBUG_FILE
    /* Initialize communication buffers (queues) */
    data = malloc(BUFFER_SIZE);
    if(data == NULL) {
        printf("Could not allocate memory for net buffer. Exiting!\n");
        return -1;
    }

    FILE* file = fopen(DEBUG_FILE_NAME, "rb");
    if(file == NULL) {
        printf("Could not open test file. Exiting!\n");
        return -1;
    }

    while(params->state != STATE_BREAK) {
        if(PaUtil_GetRingBufferWriteAvailable(&params->ringBuffer) > 0) {
            bytes = fread(data, 1, BUFFER_SIZE, file);
            if(params->writefile)
                fwrite(data, 1, bytes, wfile);
            if(bytes != -1)
                PaUtil_WriteRingBuffer(&params->ringBuffer, data, 1);

            /* For the right comparation */
            if(recv_packets++ >= USHRT_MAX)
                recv_packets = 0;
            if(params->verbose)
                printf("Reseived %d packets from remote engine.\n", recv_packets);

            /* Exit from thread by received packets */
            if(params->npack != -1 && recv_packets > params->npack)
                break;
            if(feof(file)) {
                fseek(file, 0, SEEK_SET);
            }
        } else {
            if(params->verbose)
                printf("Audio stream is not active Try wait some time...\n");
            Pa_Sleep(50);
        }
        if(check_timeout(params))
            break;
    }
    fclose(file);
#else

    unsigned char cmd[CMD_LENGTH];

    int i = 0;
    int sock = -1;
    int maxfd = -1;
    struct sockaddr_in addr;
    struct sockaddr_in clnt;
    fd_set read_fds;
    struct timeval timeout;

    /* Initialize communication buffers (queues) */
    data = malloc(MESSAGE_LENGTH);
    if(data == NULL) {
        printf("Could not allocate memory for net buffer. Exiting!\n");
        return -1;
    }

    /* Create UDP socket */
    INITSOCKET(sock)
    maxfd = sock + 1;

    /* Set socket options for reuse address */
    int optval = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval)) < 0) {
        CLOSESOCKET(sock)
        printf("Could not call setsockopt() on the server socket. Exiting!\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(params->port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Bind socket to receive data */
    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        CLOSESOCKET(sock)
        printf("Could not call bind() on the server socket.\nTry run program under root.\nExiting!\n");
        return -1;
    }

    memset(&clnt, 0, sizeof(clnt));
    clnt.sin_family = AF_INET;
    clnt.sin_port = htons(params->port);
    clnt.sin_addr.s_addr = inet_addr(params->ip);

    /* Connect to remote socket to receive data */
    if(connect(sock, (struct sockaddr *)&clnt, sizeof(clnt)) < 0) {
        CLOSESOCKET(sock)
        printf("Could not call connect() to client socket. Exiting!\n");
        return -1;
    }

    /* Constract initial message */
    if(params->mode == TEST_MODE) {
        if(params->npack != -1) {
            make_cmd(cmd, CMD_TEST_PACKETS, params->npack);
        }
        else {
            make_cmd(cmd, CMD_TEST, 0xFFFF);
        }
    }
    else if(params->mode == REAL_MODE) {
        if(params->npack != -1) {
            make_cmd(cmd, CMD_REAL_PACKETS, params->npack);
        }
        else {
            make_cmd(cmd, CMD_REAL, 0xFFFF);
        }
    }
    else {
        CLOSESOCKET(sock)
        printf("Error working mode specified. Exiting!\n");
        return -1;
    }

    /* Init remote engine message */
    if(params->verbose)
        printf("Send command: 0x%02X 0x%02X 0x%02X...\n", cmd[0], cmd[1], cmd[2]);
    if((bytes = send(sock, (char*)cmd, CMD_LENGTH, 0)) != CMD_LENGTH) {
        CLOSESOCKET(sock)
        printf("Wrong number bytes is sent in send(). Exiting!\n");
        return -1;
    }

    printf("Startup reading network audio stream...\n");

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    while(params->state != STATE_BREAK) {
        if(PaUtil_GetRingBufferWriteAvailable(&params->ringBuffer) > 0) {
            FD_ZERO(&read_fds);
            FD_SET(sock, &read_fds);
            if((bytes = select(maxfd, &read_fds, NULL, NULL, &timeout)) > 0) {
                if(FD_ISSET(sock, &read_fds)) {
                    /* Data is available */
                    if((bytes = recv(sock, data, MESSAGE_LENGTH, 0)) == MESSAGE_LENGTH) {
                        /* Convert Big-endian to Little-endian */
                        char* dataptr = (char*)(data + PREAMB_LENGTH);
                        for(i=0; i<AUDIO_LENGTH; i=i+BYTES_PER_SAMPLE) {
                            char tmp = dataptr[i + 0];
                            dataptr[i + 0] = dataptr[i + 2];
                            dataptr[i + 2] = tmp;
                        }
                        PaUtil_WriteRingBuffer(&params->ringBuffer, dataptr, 1);

                        if(params->writefile)
                            fwrite(dataptr, 1, AUDIO_LENGTH, wfile);

                        /* For the right comparation */
                        if(recv_packets++ >= USHRT_MAX)
                            recv_packets = 0;
                        //if(params->verbose)
                        //    printf("Reseived %d packets from remote engine.\n", recv_packets);
                        /* Exit from thread by received packets */
                        if(params->npack != -1 && recv_packets > params->npack)
                            break;
                    }
                    else {
                        if(params->verbose)
                            printf("Wrong number of bytes recieved: %d instead of %d\n", bytes, MESSAGE_LENGTH);
                    }
                }
            }       
        }
        else {
            if(params->verbose)
                printf("Audio stream is not active Try wait some time...\n");
            Pa_Sleep(100);
        }
        if(check_timeout(params))
            break;
    }

    /* Send message to stop remote engine */
    make_cmd(cmd, CMD_ABORT, 0x0000);
    if(params->verbose)
        printf("Send command: 0x%02X 0x%02X 0x%02X...\n", cmd[0], cmd[1], cmd[2]);
    if((bytes = sendto(sock, (char*)cmd, CMD_LENGTH, 0, (struct sockaddr*)&clnt, sizeof(clnt))) != CMD_LENGTH) {
        CLOSESOCKET(sock)
        printf("Wrong number bytes is sent in send(). Exiting!\n");
        return -1;
    }

    CLOSESOCKET(sock)
#endif

    /* Gracefull shutdown */
    printf("Close network stream.\n");
    free(data);

    if(params->writefile && wfile != NULL)
        fclose(wfile);

    return 0;
}

int send_stop(gydra_param_t* params)
{
    int bytes = -1;

    unsigned char cmd[CMD_LENGTH];

    int sock;
    struct sockaddr_in clnt;

    /* Create UDP socket */
    INITSOCKET(sock)

    memset(&clnt, 0, sizeof(clnt));
    clnt.sin_family = AF_INET;
    clnt.sin_port = htons(params->port);
    clnt.sin_addr.s_addr = inet_addr(params->ip);

    /* Send message to stop remote engine */
    make_cmd(cmd, CMD_ABORT, 0x0000);
    if(params->verbose)
        printf("Send command: 0x%02X 0x%02X 0x%02X...\n", cmd[0], cmd[1], cmd[2]);
    if((bytes = sendto(sock, (char*)cmd, CMD_LENGTH, 0, (struct sockaddr*)&clnt, sizeof(clnt))) != CMD_LENGTH) {
        CLOSESOCKET(sock)
        printf("Wrong number bytes is sent in send(). Exiting!\n");
        return -1;
    }

    CLOSESOCKET(sock)
    return 0;
}



