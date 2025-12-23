#include <stdio.h>

#include "server.h"
#include "../client/client.h"
#include "../utils.h"
#include "../cache/cache.h"

SOCKET CreateServerSocket() {
    SOCKET server_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if(server_socket == INVALID_SOCKET) {
        int err = WSAGetLastError();
        printf("socket() failed with an error code of: %d\n", err);

        return INVALID_SOCKET;
    }

    // Setup socket address server to bind to address and port (sockaddr_in is IPv4-specific, so it's needed here).
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // localhost (127.0.0.1)
    server_address.sin_port = htons(SERVER_SOCKET_PORT);
    
    if(bind(server_socket, (SOCKADDR*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        printf("bind() failed with an error code of: %d\n", err);

        closesocket(server_socket);

        return INVALID_SOCKET;
    }

    // Listen for a specific amount of connections.
    if(listen(server_socket, MAX_PENDING_CONNECTIONS) == SOCKET_ERROR) {
        int err = WSAGetLastError();
        printf("listen() failed with an error code of: %d\n", err);

        closesocket(server_socket);

        return INVALID_SOCKET;
    }

    printf("Bound server socket to and listening on: http://localhost:%d\n", SERVER_SOCKET_PORT);

    return server_socket;
}

ServerContext *NewServerContext(SOCKET server_socket, HANDLE iocp_handle) {
    ServerContext *ctx = malloc(sizeof(ServerContext));
    if(!ctx) {
        printf("Failed to allocate memory for ServerContext\n");
        return NULL;
    }
    
    ZeroMemory(ctx, sizeof(ServerContext));

    // to use the AcceptEx() function, from: https://learn.microsoft.com/en-us/windows/win32/api/mswsock/nf-mswsock-acceptex#example-code
    DWORD bytes = 0;
    GUID guid_accept_ex = WSAID_ACCEPTEX;

    int result = WSAIoctl(server_socket, 
        SIO_GET_EXTENSION_FUNCTION_POINTER, 
        &guid_accept_ex, sizeof(guid_accept_ex),
        &ctx->lpfn_accept_ex, sizeof(ctx->lpfn_accept_ex),
        &bytes, NULL, NULL
    );

    if(result == SOCKET_ERROR) {
        printf("Failed to load AcceptEx function into memory with WSAIoctl. Error Code: %d", WSAGetLastError());
        return NULL;
    }

    FileCache *cache = NewFileCache();
    if(!cache) {
        printf("Failed to create file cache.\n");
        return NULL;
    }

    ctx->cache = cache;
    ctx->iocp_handle = iocp_handle;
    ctx->server_socket = server_socket;

    return ctx;
}