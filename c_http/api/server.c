#define _GNU_SOURCE
#include <c_http/api/server.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include <c_http/constants.h>
#include <c_http/dsl/alloc.h>
#include <c_http/dsl/utils.h>

#include <c_http/logger.h>
#include <c_http/socket_functions.h>
#include <c_http/dsl/queue.h>
#include <c_http/details/worker.h>

#define TYPE Server
#define Server_TAG "SERVER"
#include <c_http/check_tag.h>
#undef TYPE
#define SERVER_DECLARE_TAG DECLARE_TAG(Server)
#define SERVER_CHECK_TAG(p) CHECK_TAG(Server, p)
#define SERVER_SET_TAG(p) SET_TAG(Server, p)

#define MAX_THREADS 100
#define XDYN_WORKER_TAB
struct Server_s {
    SERVER_DECLARE_TAG;
    int                     port;
    socket_handle_t         socket_fd;
    int                     nbr_workers;
    HandlerFunction         handler;
    QueueRef                qref;
#ifdef DYN_WORKER_TAB
    WorkerRef               *worker_tab;
#else
    WorkerRef               worker_tab[MAX_THREADS];
#endif
};


//
// create a listening socket from host and port
//
socket_handle_t create_listener_socket(int port, const char* host)
{

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    socket_handle_t tmp_socket;
    sin.sin_family = AF_INET; // or AF_INET6 (address family)
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    int result;
    int yes = 1;

    if( (tmp_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1 ) 
        goto error_01;

    // sin.sin_len = sizeof(sin);
    if( (result = setsockopt(tmp_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) != 0 )
        goto error_02;

    if( (result = bind(tmp_socket, (struct sockaddr *)&sin, sizeof(sin))) != 0)
        goto error_03;

    if((result = listen(tmp_socket, SOMAXCONN)) != 0)
        goto error_04;
    return tmp_socket;

    error_01:
        printf("socket call failed with errno %d \n", errno); 
        assert(0);
    error_02:
        printf("setsockopt call failed with errno %d \n", errno); 
        assert(0);
    error_03:
        printf("bind call failed with errno %d \n", errno); 
        assert(0);
    error_04:
        printf("listen call failed with errno %d \n", errno); 
        assert(0);
}

ServerRef Server_new(int port, int nbr_threads, HandlerFunction handler)
{
    ServerRef sref = (ServerRef)eg_alloc(sizeof(Server));
    sref->nbr_workers = nbr_threads;
    sref->port = port;
    sref->handler = handler;
    sref->qref = Queue_new();
#ifdef DYN_WORKER_TAB
    sref->worker_tab = malloc(sizeof(WorkerRef) * nbr_threads);
#else
    assert(nbr_threads < MAX_THREADS);
#endif
    SERVER_SET_TAG(sref)
    return sref;
}

void Server_dispose(ServerRef* sref)
{
    SERVER_CHECK_TAG(*sref)
    ASSERT_NOT_NULL(*sref);
    free(*sref);
    *sref = NULL;
}

void Server_listen(ServerRef sref)
{
    SERVER_SET_TAG(sref)
    ASSERT_NOT_NULL(sref)
    printf("Server_listen\n");
    //
    // Start the worker threads
    //
    for(int i = 0; i < sref->nbr_workers; i++)
    {
        WorkerRef wref = Worker_new(sref->qref, i, sref->handler);
        sref->worker_tab[i] = NULL;
        if(Worker_start(wref) != 0) {
            printf("Server failed starting thread - aborting\n");
            return;
        }
        sref->worker_tab[i] = wref;
    }
    LOG_FMT("workers started\n");
    //
    // Start a Monitor thread that will check the workers are not zombies
    //
    // Monitor   monitor{worker_v,  (int)nbr_workers};
    // monitor.start();

    // now listen  for incoming connections
    {
        int port = sref->port;
        struct sockaddr_in peername;
        unsigned int addr_length = (unsigned int)sizeof(peername);
        sref->socket_fd = create_listener_socket(port, "127.0.0.1");
        for(;;)
        {
            int sock2 = accept(sref->socket_fd, (struct sockaddr*)&peername, &addr_length);
            if( sock2 <= 0 )
            {
                LOG_FMT("%s %d", "Listener thread :: accept failed terminating sock2 : ", sock2);
                break;
            }
            LOG_FMT("Server_listener adding socket to qref %d \n", sock2);
            Queue_add(sref->qref, sock2);
        }
    }
    LOG_FMT("About to join all threads\n");
    //
    // wait for the workers to complete
    //
    for(int i = 0; i < sref->nbr_workers; i++) {
        WorkerRef wref = sref->worker_tab[i];
        if(wref != NULL) {
            LOG_FMT("About to joined worker %d\n", i);
            Worker_join(wref);
            Worker_dispose(wref);
        }
    }
    Queue_dispose(&(sref->qref));
    // also wait for the monitor  to complete
    
}
void Server_terminate(ServerRef this)
{
    SERVER_CHECK_TAG(this)
    for(int i = 0; i < this->nbr_workers; i++) {
        Queue_add(this->qref, -1);
    }
    close(this->socket_fd);
}

