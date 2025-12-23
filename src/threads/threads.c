#include <stdio.h>
#include <stdbool.h>

#include "threads.h"
#include "../client/client.h"
#include "../utils.h"

int SpawnWorkerThreads(ServerContext *ctx) {
    for(int i = 0; i < NUM_WORKER_THREADS; i++) {
        HANDLE h = CreateThread(NULL, 0, WorkerThread, (LPVOID)ctx, 0, NULL);

        if(h == NULL) {
            printf("Failed to create worker thread with an error code of: %lu\n", GetLastError());
            return -1;
        }

        // The worker threads are detached, so we don't need to keep track of the thread handles.
        if(!CloseHandle(h)) {
            printf("Failed to close worker thread handle with an error code of: %lu\n", GetLastError());
            return -1;
        }
    }

    return 0;
}

DWORD WINAPI WorkerThread(LPVOID param) {
    ServerContext *ctx = (ServerContext*)param;
    
    printf("New worker thread active...\n");

    while(true) {
        DWORD bytes = 0;
        ULONG_PTR completion_key = 0;
        LPOVERLAPPED ov = NULL;

        BOOL ok = GetQueuedCompletionStatus(ctx->iocp_handle, &bytes, &completion_key, &ov, INFINITE);

        if (!ov) {
            if (!ok && GetLastError() == WAIT_TIMEOUT) continue;
            break;
        }

        if(!ok) {
            printf("GetQueuedCompletionStatus() failed with an error code of: %lu\n", GetLastError());
            continue;
        }

        ClientContext *client_ctx = CONTAINING_RECORD(ov, ClientContext, ov);
        SOCKET client_socket = client_ctx->socket;

        switch(client_ctx->operation) {
            case OP_ACCEPT:
                if(ReceiveDataFromClient(client_socket) != 0) {
                    printf("Failed to receive data from client socket.\n");

                    closesocket(client_socket);
                    FreeClientContext(client_ctx);

                    break;
                }

                // Ready-up thread for another connection
                AcceptNewClient(ctx);

                FreeClientContext(client_ctx);
                break;
            case OP_RECV:
                // If 0 bytes get sent, the client disconnected.
                if(bytes == 0) {
                    closesocket(client_socket);
                    FreeClientContext(client_ctx);

                    break;
                }

                char *request_buffer = malloc(client_ctx->buffer_len);
                if(!request_buffer) {
                    printf("Failed to allocate memory for receive buffer from client.");
                }
                strcpy(request_buffer, client_ctx->buffer);

                if(SendDataToClient(client_socket, request_buffer, ctx) != 0) {
                    printf("Failed to send data to client.\n");

                    closesocket(client_socket);
                    FreeClientContext(client_ctx);

                    break;
                }

                FreeClientContext(client_ctx);
                break;
            case OP_SEND:
                if(bytes == 0) {
                    printf("Failed to send data to client.\n");
                    closesocket(client_socket);
                    FreeClientContext(client_ctx);

                    break;
                }

                FreeClientContext(client_ctx);
                closesocket(client_socket);
                break;
        }
    }

    return 0;
}