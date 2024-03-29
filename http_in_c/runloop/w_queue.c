#include <http_in_c/runloop/runloop.h>
#include <http_in_c//runloop/rl_internal.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

static void handler(RtorWatcherRef watcher, uint64_t event)
{
    RtorWQueueRef queue_watcher_ref = (RtorWQueueRef)watcher;
    WQUEUE_CHECK_TAG(queue_watcher_ref)
    queue_watcher_ref->queue_event_handler(queue_watcher_ref, event);
}
static void anonymous_free(RtorWatcherRef p)
{
    RtorWQueueRef queue_watcher_ref = (RtorWQueueRef)p;
    WQUEUE_CHECK_TAG(queue_watcher_ref)
    rtor_wqueue_dispose(&queue_watcher_ref);
}
void WQueue_init(RtorWQueueRef this, ReactorRef runloop, EvfdQueueRef qref)
{
    WQUEUE_SET_TAG(this);
    this->type = XR_WATCHER_QUEUE;
    this->queue = qref;
    this->fd = Evfdq_readfd(qref);
    this->runloop = runloop;
    this->free = &anonymous_free;
    this->handler = &handler;
    this->context = this;
}
RtorWQueueRef rtor_wqueue_new(ReactorRef runloop, EvfdQueueRef qref)
{
    RtorWQueueRef this = malloc(sizeof(RtorWQueue));
    WQueue_init(this, runloop, qref);
    return this;
}
void rtor_wqueue_dispose(RtorWQueueRef* athis)
{
    WQUEUE_CHECK_TAG(*athis)
    close((*athis)->fd);
    free((void*)*athis);
    *athis = NULL;
}
void rtor_wqueue_register(RtorWQueueRef athis, QueueEventHandler cb, void* arg)
{
//    WQUEUE_CHECK_TAG(this)
    uint64_t interest = EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLHUP;

//    uint32_t interest = watch_what;
    athis->queue_event_handler = cb;
    athis->queue_event_handler_arg = arg;
    int res = rtor_reactor_register(athis->runloop, athis->fd, interest, (RtorWatcherRef) (athis));
    assert(res ==0);
}
void rtor_wqueue_change_watch(RtorWQueueRef athis, QueueEventHandler cb, void* arg, uint64_t watch_what)
{
    WQUEUE_CHECK_TAG(athis)
    uint32_t interest = watch_what;
    if(cb != NULL) {
        athis->queue_event_handler = cb;
    }
    if (arg != NULL) {
        athis->queue_event_handler_arg = arg;
    }
    int res = rtor_reactor_reregister(athis->runloop, athis->fd, interest, (RtorWatcherRef) athis);
    assert(res == 0);
}
void rtor_wqueue_deregister(RtorWQueueRef athis)
{
    WQUEUE_CHECK_TAG(athis)

    int res = rtor_reactor_deregister(athis->runloop, athis->fd);
    assert(res == 0);
}
ReactorRef rtor_wqueue_get_reactor(RtorWQueueRef athis)
{
    return athis->runloop;
}
int rtor_wqueue_get_fd(RtorWQueueRef this)
{
    return this->fd;
}

void rtor_wqueue_verify(RtorWQueueRef r)
{
    WQUEUE_CHECK_TAG(r)

}
