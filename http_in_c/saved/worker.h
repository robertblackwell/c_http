#ifndef c_http_sync_worker_h
#define c_http_sync_worker_h
//
#include <http_in_c/common/queue.h>
#include <http_in_c/saved/sync_handler.h>
#include <pthread.h>
struct Worker_s;
typedef struct Worker_s Worker, *WorkerRef;

typedef void(worker_on_message_handler(MessageRef message_ref, void* context));

WorkerRef Worker_new(QueueRef qref, int ident, size_t read_buffer_size, SyncAppMessageHandler app_handler);
void Worker_dispose(WorkerRef wref);
int Worker_start(WorkerRef wref);
pthread_t* Worker_pthread(WorkerRef wref);
int Worker_socketfd(WorkerRef wref);
void Worker_join(WorkerRef wref);

#endif