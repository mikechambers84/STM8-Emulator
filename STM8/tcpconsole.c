#include <stdio.h>
#include <stdint.h>
#include <winsock2.h>
#include "peripherals/uart1.h"
#include "peripherals/uart3.h"

WSADATA wsaData;
SOCKET listen_socket[3], msgsock[3];
char* ip_address = NULL;

int fromlen[3];
int socket_type = SOCK_STREAM;
struct sockaddr_in local[3], from[3];
uint8_t didstartup = 0;

int tcpconsole_init(uint8_t uart, uint16_t port) {
    int retval;
    unsigned long iMode = 1;

    if (!didstartup) {
        // Request Winsock version 2.2
        if ((retval = WSAStartup(0x202, &wsaData)) != 0) {
            fprintf(stderr, "Server: WSAStartup() failed with error %d\n", retval);
            WSACleanup();
            return -1;
        }
        didstartup = 1;
    }

    local[uart].sin_family = AF_INET;
    local[uart].sin_addr.s_addr = (!ip_address) ? INADDR_ANY : inet_addr(ip_address);
    local[uart].sin_port = htons(port);
    listen_socket[uart] = socket(AF_INET, socket_type, 0);

    if (listen_socket[uart] == INVALID_SOCKET) {
        fprintf(stderr, "Server: socket() failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    if (bind(listen_socket[uart], (struct sockaddr*)&local[uart], sizeof(local[uart])) == SOCKET_ERROR) {
        fprintf(stderr, "Server: bind() failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    if (listen(listen_socket[uart], 5) == SOCKET_ERROR) {
        fprintf(stderr, "Server: listen() failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    ioctlsocket(listen_socket[uart], FIONBIO, &iMode); //we want accept() to not block

    msgsock[uart] = INVALID_SOCKET;
    return 0;
}

void tcpconsole_dorecv(uint8_t uart) {
    int actuallen = 0;
    uint8_t val;
    unsigned long iMode = 1;

    if (msgsock[uart] == INVALID_SOCKET) {
        fromlen[uart] = sizeof(from[uart]);
        msgsock[uart] = accept(listen_socket[uart], (struct sockaddr*)&from[uart], &fromlen[uart]);
        ioctlsocket(msgsock[uart], FIONBIO, &iMode); //non-blocking IO
    }

    if (msgsock[uart] == INVALID_SOCKET) return;
    actuallen = recv(msgsock[uart], &val, 1, 0);
    if (actuallen > 0) {
        //printf("%c", val);
        if (uart == 0)
            uart1_rxBufAdd(val);
        else if (uart == 2)
            uart3_rxBufAdd(val);
    }
    else if (actuallen == 0) { //graceful close
        closesocket(msgsock[uart]);
        msgsock[uart] = INVALID_SOCKET;
    }
    else {
        //printf("%d\n", WSAGetLastError());
        switch (WSAGetLastError()) {
        case WSAEWOULDBLOCK: //this is ok, just no data yet
            break;
        case WSANOTINITIALISED: //the rest of these are bad...
        case WSAENETDOWN:
        case WSAENOTCONN:
        case WSAENETRESET:
        case WSAENOTSOCK:
        case WSAESHUTDOWN:
        case WSAEINVAL:
        case WSAECONNABORTED:
        case WSAETIMEDOUT:
        case WSAECONNRESET:
            closesocket(msgsock[uart]);
            msgsock[uart] = INVALID_SOCKET;
            break;
        default:
            break;
        }
    }
}

void tcpconsole_loop() {
    if (uart1_redirect == UART1_REDIRECT_TCP) {
        tcpconsole_dorecv(0);
    }
    if (uart3_redirect == UART3_REDIRECT_TCP) {
        tcpconsole_dorecv(2);
    }
}

void tcpconsole_send(uint8_t uart, uint8_t val) {
    int ret;
    if (msgsock[uart] == INVALID_SOCKET) return;
    ret = send(msgsock[uart], &val, 1, 0);
    //printf("send %d\n", ret);
    if (ret < 1) {
        //TODO: error handling
    }
}
