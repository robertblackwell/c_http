#include "demo_process_main.h"

#include <http_in_c/demo_protocol/demo_server.h>
#include <http_in_c/demo_protocol/demo_message.h>
#include <http_in_c/common/socket_functions.h>
#include <rbl/logger.h>
#include <stdio.h>
#include <mcheck.h>
#include <sys/types.h>
#include <sys/wait.h>
#include<signal.h>

#define MAX_NBR_THREADS 10
void* thread_function(void* arg);

typedef struct ThreadContext_s {
    int             ident;
    pthread_t       thread;
    int             port;
    const char*     host;
    int             listening_socket;
    void*           return_value;
    DemoServerRef   server_ref;
} ThreadContext;


void demo_process_main(char* host, int port, int nbr_threads, int nbr_connections_per_thread, int nbr_rountrips_per_connection)
{
    ThreadContext thread_table[nbr_threads];
    assert(nbr_threads <= MAX_NBR_THREADS);
    printf("Process starting pid: %d\n", getpid());
    for(int i = 0; i < nbr_threads; i++) {
        ThreadContext* ctx = &(thread_table[i]);
        ctx->ident = i;
        ctx->port = port;
        ctx->host = host;
        pthread_create(&(ctx->thread), NULL, thread_function, ctx);
    }
    for(int i = 0; i < nbr_threads; i++) {
        ThreadContext* ctx = &(thread_table[i]);
        pthread_join(ctx->thread, (void**)&(ctx->return_value));
    }
}
void* thread_function(void* arg)
{
    ThreadContext* ctx = arg;
    int listening_socket_fd = create_listener_socket(ctx->port, ctx->host);
    printf("thread pid: %d tid: %d host: %s port: %d ident: %d pthread_t: %lu listening_socket: %d\n", getpid(), gettid(), ctx->host, ctx->port, ctx->ident, ctx->thread, ctx->listening_socket);
    ctx->server_ref = DemoServer_new(ctx->port, ctx->host, listening_socket_fd, NULL);
    DemoServer_listen(ctx->server_ref);
    runloop_run(ctx->server_ref->runloop_ref, -1 /* infinite*/);
    DemoServer_free(ctx->server_ref);
    ctx->server_ref = NULL;
    return NULL;
}
