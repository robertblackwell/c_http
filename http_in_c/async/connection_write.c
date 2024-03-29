

#define CHLOG_ON
#include <http_in_c/async/connection_internal.h>

static void writer(AsyncConnectionRef connection_ref);
static void write_error(AsyncConnectionRef connection_ref, char* msg);
static void post_postable_writer(AsyncConnectionRef cref);
static void write_complete(AsyncConnectionRef cref);
static void write_incomplete(AsyncConnectionRef cref);

/**
 * Utility function that wraps all rtor_reactor_post() calls so this module can
 * keep track of outstanding pending function calls
 */

/////////////////////////////////////////////////////////////////////////////////////
// write sequence - sequence of functions called during write operation
////////////////////////////////////////////////////////////////////////////////////
void post_postable_writer(AsyncConnectionRef cref)
{
    rtor_reactor_post(cref->reactor_ref, &async_postable_writer, cref);
}
void async_postable_writer(ReactorRef reactor_ref, void* arg)
{
    AsyncConnectionRef connection_ref = arg;
    CHECK_TAG(AsyncConnection_TAG, connection_ref)
    CHTTP_ASSERT((connection_ref->active_output_buffer_ref != NULL), "post_write_handler");
    CHTTP_ASSERT((connection_ref->write_state == WRITE_STATE_ACTIVE), "expect write_state to be active");
    writer(connection_ref);
}

static void writer(AsyncConnectionRef cref)
{
    CHECK_TAG(AsyncConnection_TAG, cref)
    CHTTP_ASSERT((cref->active_output_buffer_ref != NULL), "writer");
    IOBufferRef iob = cref->active_output_buffer_ref;
    int buflen = IOBuffer_data_len(iob);
    long nsend = send(cref->socket_stream_ref->fd, IOBuffer_data(iob), IOBuffer_data_len(iob), MSG_DONTWAIT);
    int errno_saved = errno;
    if(nsend > 0) {
        IOBuffer_consume(iob, nsend);
        if(IOBuffer_data_len(iob) == 0) {
            // write is complete
            cref->write_state = WRITE_STATE_IDLE;
            IOBuffer_dispose(&(cref->active_output_buffer_ref));
            cref->active_output_buffer_ref = NULL;
            write_complete(cref);
        } else {
            // write not complete schedule another try
            printf("writer errno_saved %d %s\n", errno_saved, strerror(errno_saved));
            cref->write_state = WRITE_STATE_ACTIVE;
            write_incomplete(cref);
        }
    } else if (nsend == 0) {
        write_error(cref, "think the fd is closed by other end");
    } else if ((nsend == -1) && (errno == EAGAIN)) {
        cref->write_state = WRITE_STATE_EAGAINED;
        return;
    } else if(nsend == -1) {
        write_error(cref, "think this was an io error");
    }
}
/////////////////////////////////////////////////////////////////////////////
static void write_incomplete(AsyncConnectionRef cref)
{
    AsyncHandlerRef href = cref->handler_ref;
    CHECK_TAG(AsyncConnection_TAG, cref)
    CHECK_TAG(AsyncHandler_TAG, href);
    post_postable_writer(cref);
}
//////////////////////////////////////////////////////////////////////////
void postable_write_complete(ReactorRef reactor_ref, void* arg);
static void write_complete(AsyncConnectionRef cref)
{
    AsyncHandlerRef href = cref->handler_ref;
    CHECK_TAG(AsyncConnection_TAG, cref)
    CHECK_TAG(AsyncHandler_TAG, href);
    rtor_reactor_post(cref->reactor_ref, &postable_write_complete, cref);
}
void postable_write_complete(ReactorRef reactor_ref, void* arg)
{
    AsyncConnectionRef connection_ref = arg;
    AsyncHandlerRef href = connection_ref->handler_ref;
    CHECK_TAG(AsyncConnection_TAG, connection_ref)
    CHECK_TAG(AsyncHandler_TAG, href);
    href->handle_write_complete(href);
}
////////////////////////////////////////////////////////////
static void postable_close_connection(ReactorRef reactor_ref, void* arg);
static void write_error(AsyncConnectionRef cref, char* msg)
{
    CHECK_TAG(AsyncConnection_TAG, cref)
    AsyncHandlerRef href = cref->handler_ref;
    CHECK_TAG(AsyncHandler_TAG, href)
    LOG_FMT("Write_error got an error this is the message: %s  fd: %d", msg, cref->socket_stream_ref->fd);
    cref->read_state = READ_STATE_STOP;
    cref->write_state = WRITE_STATE_STOP;
    rtor_reactor_post(cref->reactor_ref, &postable_close_connection, href);
}
static void postable_close_connection(ReactorRef reactor_ref, void* arg)
{
    AsyncHandlerRef href = arg;
    CHECK_TAG(AsyncHandler_TAG, href);
    AsyncConnectionRef cref = href->async_connection_ref;
    href->handle_close_connection(href);
}
