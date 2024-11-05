#include <http_in_c//runloop/runloop.h>
#include <http_in_c/runloop/rl_internal.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

/**
 * Called whenever an fd associated with an WSocket receives an fd event.
 * Should dispatch the read_evhandler and/or write_evhandler depending on whether those
 * events (read events and write events) are armed.
 * @param ctx       void*
 * @param fd        int
 * @param event     uint64_t
 */
static void handler(RunloopWatcherRef watcher, uint64_t event)
{
    RunloopStreamRef sw = (RunloopStreamRef)watcher;
    RunloopRef rl = watcher->runloop;
    /*
     * These should be posted not called
     */
//    if(sw->both_postable_cb) {
//        sw->both_postable_cb(rl, sw->both_postable_arg);
//    }
    if((sw->event_mask & EPOLLIN) && (sw->read_postable_cb)) {
        sw->read_postable_cb(rl, sw->read_postable_arg);
    }
    if((sw->event_mask & EPOLLOUT) && (sw->write_postable_cb)) {
        sw->write_postable_cb(rl, sw->write_postable_arg);
    }
}

static void anonymous_free(RunloopWatcherRef p)
{
    RunloopStreamRef twp = (RunloopStreamRef)p;
    runloop_stream_free(twp);
}
void runloop_stream_init(RunloopStreamRef this, RunloopRef runloop, int fd)
{
    SOCKW_SET_TAG(this);
    this->fd = fd;
    this->runloop = runloop;
    this->free = &anonymous_free;
    this->handler = &handler;
    this->event_mask = 0;
    this->read_postable_arg = NULL;
    this->read_postable_cb = NULL;
    this->write_postable_arg = NULL;
    this->write_postable_cb = NULL;
}
RunloopStreamRef runloop_stream_new(RunloopRef runloop, int fd)
{
    RunloopStreamRef this = malloc(sizeof(RunloopStream));
    runloop_stream_init(this, runloop, fd);
    return this;
}
void runloop_stream_free(RunloopStreamRef athis)
{
    SOCKW_CHECK_TAG(athis)
    close(athis->fd);
    free((void*)athis);
}
void runloop_stream_register(RunloopStreamRef athis)
{
    SOCKW_CHECK_TAG(athis)
    uint32_t interest = 0;
    int res = runloop_register(athis->runloop, athis->fd, 0L, (RunloopWatcherRef) (athis));
    assert(res ==0);
}
//void WIoFd_change_watch(RunloopStreamRef this, SocketEventHandler cb, void* arg, uint64_t watch_what)
//{
//    SOCKW_CHECK_TAG(this)
//    uint32_t interest = watch_what;
//    if( cb != NULL) {
//        this->cb = cb;
//    }
//    if (arg != NULL) {
//        this->cb_ctx = arg;
//    }
//    int res = runloop_reregister(this->runloop, this->fd, interest, (RunloopWatcherRef)this);
//    assert(res == 0);
//}
void runloop_stream_deregister(RunloopStreamRef athis)
{
    SOCKW_CHECK_TAG(athis)
    int res = runloop_deregister(athis->runloop, athis->fd);
    assert(res == 0);
}
void runloop_stream_arm_both(RunloopStreamRef athis,
                             PostableFunction read_postable_cb, void* read_arg,
                             PostableFunction write_postable_cb, void* write_arg)
{
    uint64_t interest = EPOLLET | EPOLLOUT | EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP | athis->event_mask;
    athis->event_mask = interest;
    SOCKW_CHECK_TAG(athis)
    if(read_postable_cb != NULL) {
        athis->read_postable_cb = read_postable_cb;
    }
    if (read_arg != NULL) {
        athis->read_postable_arg = read_arg;
    }
    if(write_postable_cb != NULL) {
        athis->write_postable_cb = write_postable_cb;
    }
    if (write_arg != NULL) {
        athis->write_postable_arg = write_arg;
    }
    int res = runloop_reregister(athis->runloop, athis->fd, interest, (RunloopWatcherRef) athis);
    assert(res == 0);
}

void runloop_stream_arm_read(RunloopStreamRef athis, PostableFunction postable_cb, void* arg)
{
    uint64_t interest = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP | athis->event_mask;
    athis->event_mask = interest;
    SOCKW_CHECK_TAG(athis)
    if(postable_cb != NULL) {
        athis->read_postable_cb = postable_cb;
    }
    if (arg != NULL) {
        athis->read_postable_arg = arg;
    }
    int res = runloop_reregister(athis->runloop, athis->fd, interest, (RunloopWatcherRef) athis);
    assert(res == 0);
}
void runloop_stream_arm_write(RunloopStreamRef athis, PostableFunction postable_cb, void* arg)
{
    uint64_t interest = EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | athis->event_mask;
    athis->event_mask = interest;
    SOCKW_CHECK_TAG(athis)
    if(postable_cb != NULL) {
        athis->write_postable_cb = postable_cb;
    }
    if (arg != NULL) {
        athis->write_postable_arg = arg;
    }
    int res = runloop_reregister(athis->runloop, athis->fd, interest, (RunloopWatcherRef) athis);
    assert(res == 0);
}
void runloop_stream_disarm_read(RunloopStreamRef athis)
{
    athis->event_mask &= ~EPOLLIN;
    SOCKW_CHECK_TAG(athis)
    athis->read_postable_cb = NULL;
    athis->read_postable_arg = NULL;
    int res = runloop_reregister(athis->runloop, athis->fd, athis->event_mask, (RunloopWatcherRef) athis);
    assert(res == 0);
}
void runloop_stream_disarm_write(RunloopStreamRef athis)
{
    athis->event_mask = ~EPOLLOUT & athis->event_mask;
    SOCKW_CHECK_TAG(athis)
    athis->write_postable_cb = NULL;
    athis->write_postable_arg = NULL;
    int res = runloop_reregister(athis->runloop, athis->fd, athis->event_mask, (RunloopWatcherRef) athis);
    assert(res == 0);
}
RunloopRef runloop_stream_get_reactor(RunloopStreamRef athis)
{
    return athis->runloop;
}
int runloop_stream_get_fd(RunloopStreamRef this)
{
    return this->fd;
}

void runloop_stream_verify(RunloopStreamRef r)
{
    SOCKW_CHECK_TAG(r)

}


