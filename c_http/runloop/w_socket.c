#include <c_http/runloop/w_socket.h>
#include <assert.h>
#include <stdbool.h>
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
static void handler(WatcherRef watcher, int fd, uint64_t event)
{
    WSocketRef sw = (WSocketRef)watcher;
    assert(fd == sw->fd);
    if((sw->event_mask & EPOLLIN) && (sw->read_evhandler)) {
        sw->read_evhandler(sw, sw->read_arg, event);
    }
    if((sw->event_mask & EPOLLOUT) && (sw->write_evhandler)) {
        sw->write_evhandler(sw, sw->write_arg, event);
    }
//    sw->cb((WatcherRef)sw, sw->cb_ctx, event);
}
static void anonymous_free(WatcherRef p)
{
    WSocketRef twp = (WSocketRef)p;
    WSocket_free(twp);
}
void WSocket_init(WSocketRef this, XrReactorRef runloop, int fd)
{
    this->type = XR_WATCHER_SOCKET;
    sprintf(this->tag, "XRSW");
    XR_SOCKW_SET_TAG(this);
    this->fd = fd;
    this->runloop = runloop;
    this->free = &anonymous_free;
    this->handler = &handler;
    this->event_mask = 0;
    this->read_arg = NULL;
    this->read_evhandler = NULL;
    this->write_arg = NULL;
    this->write_evhandler = NULL;
}
WSocketRef WSocket_new(XrReactorRef rtor_ref, int fd)
{
    WSocketRef this = malloc(sizeof(WSocket));
    WSocket_init(this, rtor_ref, fd);
    return this;
}
void WSocket_free(WSocketRef this)
{
    XRSW_TYPE_CHECK(this)
    XR_SOCKW_CHECK_TAG(this)
    close(this->fd);
    free((void*)this);
}
void WSocket_register(WSocketRef this)
{
    XRSW_TYPE_CHECK(this)
    XR_SOCKW_CHECK_TAG(this)

    uint32_t interest = 0;
    int res = XrReactor_register(this->runloop, this->fd, 0L, (WatcherRef)(this));
    assert(res ==0);
}
//void WSocket_change_watch(WSocketRef this, SocketEventHandler cb, void* arg, uint64_t watch_what)
//{
//    XR_SOCKW_CHECK_TAG(this)
//    uint32_t interest = watch_what;
//    if( cb != NULL) {
//        this->cb = cb;
//    }
//    if (arg != NULL) {
//        this->cb_ctx = arg;
//    }
//    int res = XrReactor_reregister(this->runloop, this->fd, interest, (WatcherRef)this);
//    assert(res == 0);
//}
void WSocket_deregister(WSocketRef this)
{
    XRSW_TYPE_CHECK(this)
    XR_SOCKW_CHECK_TAG(this)

    int res =  XrReactor_deregister(this->runloop, this->fd);
    assert(res == 0);
}
void WSocket_arm_read(WSocketRef this, SocketEventHandler fd_event_handler, void* arg)
{
    uint64_t interest = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP | this->event_mask;
    this->event_mask = interest;
    XR_SOCKW_CHECK_TAG(this)
    if( fd_event_handler != NULL) {
        this->read_evhandler = fd_event_handler;
    }
    if (arg != NULL) {
        this->read_arg = arg;
    }
    int res = XrReactor_reregister(this->runloop, this->fd, interest, (WatcherRef)this);
    assert(res == 0);
}
void WSocket_arm_write(WSocketRef this, SocketEventHandler fd_event_handler, void* arg)
{
    uint64_t interest = EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | this->event_mask;
    this->event_mask = interest;
    XR_SOCKW_CHECK_TAG(this)
    if( fd_event_handler != NULL) {
        this->write_evhandler = fd_event_handler;
    }
    if (arg != NULL) {
        this->write_arg = arg;
    }
    int res = XrReactor_reregister(this->runloop, this->fd, interest, (WatcherRef)this);
    assert(res == 0);
}
void WSocket_disarm_read(WSocketRef this)
{

}
void WSocket_disarm_write(WSocketRef this)
{

}


