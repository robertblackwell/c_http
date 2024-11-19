#define RBL_LOG_ENABLED
#define RBL_LOG_ALLOW_GLOBAL
#include <http_in_c/demo_protocol/demo_connection.h>
#include <http_in_c/runloop/rl_internal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <rbl/macros.h>
#include <rbl/logger.h>
#include <http_in_c/runloop/runloop.h>
#include <http_in_c/demo_protocol/demo_message.h>

#define READ_STATE_IDLE     11
#define READ_STATE_ACTIVE   13
#define READ_STATE_STOP     14

#define WRITE_STATE_IDLE     21
#define WRITE_STATE_ACTIVE   23
#define WRITE_STATE_STOP     24

static void read_start(DemoConnectionRef cref);
static void read_start_postable(RunloopRef rl, void* cref_arg);
static void postable_read_start(RunloopRef runloop_ref, void* arg);
static void on_parser_new_message_complete(void* arg_ctx, DemoMessageRef msg);
static void on_read_complete(DemoConnectionRef cref, DemoMessageRef msg, int error_code);
static void read_error(DemoConnectionRef cref, char* msg);

static void postable_writer(RunloopRef runloop_ref, void* arg);
static void postable_write_call_cb(RunloopRef runloop_ref, void* arg);
static void writer_cb(void* cref_arg, long bytes_written, int status);
static void write_error(DemoConnectionRef cref, char* msg);

static void postable_cleanup(RunloopRef runloop, void* cref);
static void read_have_data_cb(void* cref_arg, long bytes_available, int err_status);
static void read_process_data(DemoConnectionRef cref);
/**
 * Utility function that wraps all runloop_post() calls so this module can
 * keep track of outstanding pending function calls
 */
static void post_to_runloop(DemoConnectionRef cref, void(*postable_function)(RunloopRef, void*));


DemoConnectionRef democonnection_new(
        RunloopRef runloop_ref,
        int socket,
        void(*connection_completion_cb)(void* href),
        void* handler_ref
)
{
    DemoConnectionRef this = malloc(sizeof(DemoConnection));
    democonnection_init(this, runloop_ref, socket, connection_completion_cb, handler_ref);
    return this;
}
void democonnection_init(
        DemoConnectionRef this,
        RunloopRef runloop_ref,
        int socket,
        void(*connection_completion_cb)(void* href),
        void* handler_ref
        )
{
    RBL_SET_TAG(DemoConnection_TAG, this)
    RBL_SET_END_TAG(DemoConnection_TAG, this)
    RBL_CHECK_TAG(DemoConnection_TAG, this)
    RBL_CHECK_END_TAG(DemoConnection_TAG, this)
    this->runloop_ref = runloop_ref;
    this->asio_stream_ref = asio_stream_new(runloop_ref, socket);
    this->handler_ref = handler_ref;
    this->read_buffer_size = 1000;
    this->input_message_list_ref = List_new();
    this->active_input_buffer_ref = NULL;
    this->active_output_buffer_ref = NULL;
    this->read_state = READ_STATE_IDLE;
    this->write_state = WRITE_STATE_IDLE;
    this->on_write_cb = NULL;
    this->on_write_cb_arg = NULL;
    this->on_read_cb = NULL;
    this->on_read_cb_arg =  NULL;
    this->cleanup_done_flag = false;
    this->on_close_cb = connection_completion_cb;
    this->on_close_cb_arg = handler_ref;
    this->parser_ref = DemoParser_new(
            (void*)&on_parser_new_message_complete,
            this);
}
void democonnection_close(DemoConnectionRef cref)
{
    asio_stream_close(cref->asio_stream_ref);
}
void democonnection_free(DemoConnectionRef this)
{
    RBL_CHECK_TAG(DemoConnection_TAG, this)
    RBL_CHECK_END_TAG(DemoConnection_TAG, this)
    int fd = this->asio_stream_ref->fd;
    close(fd);
    asio_stream_free(this->asio_stream_ref);
    this->asio_stream_ref = NULL;
    DemoParser_free(this->parser_ref);
    List_safe_free(this->input_message_list_ref, demo_message_anonymous_free);
    if(this->active_output_buffer_ref) {
        IOBuffer_free(this->active_output_buffer_ref);
        this->active_output_buffer_ref = NULL;
    }
    if(this->active_input_buffer_ref) {
        IOBuffer_free(this->active_input_buffer_ref);
        this->active_input_buffer_ref = NULL;
    }
    free(this);
}
static void call_on_write_cb(DemoConnectionRef cref, int status);
static void call_on_read_cb(DemoConnectionRef cref, DemoMessageRef msgref, int status);

/////////////////////////////////////////////////////////////////////////////////////
// read sequence - sequence of functions called processing a read operation
////////////////////////////////////////////////////////////////////////////////////
void democonnection_read(DemoConnectionRef cref, void(*on_demo_read_cb)(void* href, DemoMessageRef, int status), void* href) {
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    RBL_LOG_FMT("democonnect_read fd: %d", cref->asio_stream_ref->runloop_stream_ref->fd);
    assert(on_demo_read_cb != NULL);

    // should be in READ_STATE_IDLE - check the variable values for state
    assert(cref->read_state == READ_STATE_IDLE);
    assert(cref->active_input_buffer_ref == NULL);
    assert(cref->on_read_cb == NULL);
    assert(cref->on_read_cb_arg == NULL);

    cref->on_read_cb = on_demo_read_cb;
    cref->on_read_cb_arg = href;
    if (List_size(cref->input_message_list_ref) > 1) {
        // this means the client is pipelining
        // should not be  happening
        DemoMessageRef msgref = List_remove_first(cref->input_message_list_ref);
        call_on_read_cb(cref, msgref, 0);
    } else if (List_size(cref->input_message_list_ref) == 1) {
        DemoMessageRef msgref = List_remove_first(cref->input_message_list_ref);
        cref->read_state = READ_STATE_ACTIVE;
        call_on_read_cb(cref, msgref, 0);
    } else {
        // nothing in the input ueue so start a read operation
        cref->read_state = READ_STATE_ACTIVE;
        assert(cref->active_input_buffer_ref == NULL);
        if (cref->active_input_buffer_ref == NULL) {
            cref->active_input_buffer_ref = IOBuffer_new_with_capacity((int) cref->read_buffer_size);
        }
        assert(IOBuffer_data_len(cref->active_input_buffer_ref) == 0);

        if (IOBuffer_data_len(cref->active_input_buffer_ref) == 0) {
            IOBufferRef iob = cref->active_input_buffer_ref;
            void *input_buffer_ptr = IOBuffer_space(iob);
            int input_buffer_length = IOBuffer_space_len(iob);
            asio_stream_read(cref->asio_stream_ref, input_buffer_ptr, input_buffer_length, &read_have_data_cb, cref);
        } else {
            // active_input_buffer should be fully processed  and free() at the end of the previous rread
            assert(false);
        }
    }
}
/**
 * Expects:
 * -    read state READ_STATE_ACTIVE
 * -    active_input_buffer_ref != NULL
 * -    if err_status == 0
 * -        bytes_available >= 0
 * -    else
 * -        bytes_available < 0
 */
static void read_have_data_cb(void* cref_arg, long bytes_available, int err_status)
{
    DemoConnectionRef cref = cref_arg;
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    assert(cref->active_input_buffer_ref != NULL);
    IOBufferRef iob = cref->active_input_buffer_ref;
    RBL_LOG_FMT("cref_arg: %p bytes_available: %ld status: %d", cref_arg, bytes_available, err_status);
    if(bytes_available == 0) {
        /**
         *  bytes_available == 0 signals the client closed the connection.
         *  Clients are not permitted to end a valid message in this way so
         *  treat this the same as an IO error.
         *  Abandon all waiting input messages and Close the connection
         */
        call_on_read_cb(cref, NULL, -1);
    } else if(bytes_available < 0) {
        call_on_read_cb(cref, NULL, -2);
    } else {//   (bytes_available > 0)
        IOBuffer_commit(iob, (int)bytes_available);
        RBL_LOG_FMT("Before DemoParser_consume read_state %d", cref->read_state);

        /**
         * This call will put any new messages onto the input message queue(cref->input_message_list_ref)
         *
         * Further more the call will consume all of the buffer unless there is a parsing error
         *
         * A parsing error will be treated as an IO error and the connection will be shutdown.
         *
         * Hence
         * -    If the return value is 0 dispose the buffer, set READ_STATE_IDLE and call the read callback
         *
         * -    if there is an error dispose the buffer(we wiwont need it),
         *      set READ_STATE_STOP and start error processing.
         *      -   there is an alternative approach, namely
         *          -   set READ_STATE_IDLE,
         *          -   pass the error code back to th caller and let them decide how to proceed.
         *
         */
        int status = DemoParser_consume(cref->parser_ref, iob);

        assert(cref->active_input_buffer_ref != NULL);
        assert(IOBuffer_data_len(cref->active_input_buffer_ref) == 0);
        IOBuffer_free(cref->active_input_buffer_ref);
        cref->active_input_buffer_ref = NULL;

        if(status == 0) {
            if(List_size(cref->input_message_list_ref) > 0) {
                DemoMessageRef msg = List_remove_first(cref->input_message_list_ref);
                cref->read_state = READ_STATE_IDLE;
                call_on_read_cb(cref, msg, 0);
            }
        } else {
            /**
             *
             * @TODO fix
            const char * etext = demo_message_error_name(status);
             */
            read_error(cref, "");
        }
    }
}

/**
 * This function is called by the underlying parser when a new message is complete.
 *
 * The new message is added to the connection input message queue(input_message_list_ref)
 */
static void on_parser_new_message_complete(void* arg_ctx, DemoMessageRef msg_ref)
{
    RBL_LOG_FMT("arg_ctx %p msg_ref: %p", arg_ctx, msg_ref)
    DemoConnectionRef cref = arg_ctx;
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    List_add_back(cref->input_message_list_ref, msg_ref);
}
static void postable_call_on_read_cb(RunloopRef runloop_ref, void* arg_cref)
{
    DemoConnectionRef cref = arg_cref;
    assert(List_size(cref->input_message_list_ref) > 0);
    DemoMessageRef msgref = List_remove_first(cref->input_message_list_ref);
    cref->read_state = READ_STATE_IDLE;
    call_on_read_cb(cref, msgref, 0);
}
static void call_on_read_cb(DemoConnectionRef cref, DemoMessageRef msg, int status)
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    RBL_ASSERT((cref->on_read_cb != NULL), "write call back is NULL");
    RBL_ASSERT((cref->on_read_cb_arg != NULL), "write call arg back is NULL");
    void(*tmp)(void*, DemoMessageRef, int) = cref->on_read_cb;
    void* arg = cref->on_read_cb_arg;
    cref->on_read_cb = NULL;
    cref->on_read_cb_arg = NULL;
    tmp(arg, msg, status);
}
/////////////////////////////////////////////////////////////////////////////////////
// end of read sequence
/////////////////////////////////////////////////////////////////////////////////////
// write sequence - sequence of functions called during write operation
////////////////////////////////////////////////////////////////////////////////////
void democonnection_write(
        DemoConnectionRef cref,
        DemoMessageRef response_ref,
        void(*on_demo_write_cb)(void* href, int status),
        void* href)
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    RBL_ASSERT((response_ref != NULL), "got NULL instead of a response_ref");
    RBL_ASSERT((on_demo_write_cb != NULL), "got NULL for on_demo_write_cb");
    RBL_ASSERT((cref->write_state == WRITE_STATE_IDLE), "a write is already active");
    RBL_ASSERT((cref->on_write_cb == NULL), "there is already a write in progress");
    RBL_ASSERT((cref->active_output_buffer_ref == NULL),"something wrong active output buffer should be null")
    cref->on_write_cb = on_demo_write_cb;
    cref->on_write_cb_arg = href;
    cref->write_state == WRITE_STATE_ACTIVE;
    cref->active_output_buffer_ref = demo_message_serialize(response_ref);
    if(cref->write_state == WRITE_STATE_STOP) {
        call_on_write_cb(cref, DemoConnectionErrCode_is_closed);
        return;
    }
    post_to_runloop(cref, &postable_writer);
}

static void postable_writer(RunloopRef runloop_ref, void* arg)
{
    DemoConnectionRef cref = arg;
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    if(cref->write_state == WRITE_STATE_STOP) {
        IOBuffer_free(cref->active_output_buffer_ref);
        cref->active_output_buffer_ref = NULL;
        call_on_write_cb(cref, DemoConnectionErrCode_is_closed);
        return;
    }
    RBL_ASSERT((cref->active_output_buffer_ref != NULL), "post_write_handler");
    RBL_ASSERT((cref->write_state == WRITE_STATE_IDLE), "cannot call write while write state is not idle");
    IOBufferRef iob = cref->active_output_buffer_ref;
    void* buf = IOBuffer_data(iob);
    long length = IOBuffer_data_len(iob);
    asio_stream_write(cref->asio_stream_ref, buf, length, &writer_cb, cref);
}

static void writer_cb(void* cref_arg, long bytes_written, int status)
{
    DemoConnectionRef cref = cref_arg;
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    RBL_ASSERT((cref->active_output_buffer_ref != NULL), "writer");
    IOBufferRef iob = cref->active_output_buffer_ref;
    RBL_ASSERT((bytes_written > 0),"should not get here is no bytes written");
    IOBuffer_consume(iob, (int)bytes_written);
    if(IOBuffer_data_len(iob) > 0) {
        runloop_post(cref->runloop_ref, postable_writer, cref);
    } else {
        IOBuffer_free(cref->active_output_buffer_ref);
        cref->active_output_buffer_ref = NULL;
        post_to_runloop(cref, &postable_write_call_cb);
    }
}
static void post_to_runloop(DemoConnectionRef cref, void(*postable_function)(RunloopRef, void*))
{
    runloop_post(cref->runloop_ref, postable_function, cref);
}
static void postable_write_call_cb(RunloopRef runloop_ref, void* arg)
{
    DemoConnectionRef cref = arg;
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    call_on_write_cb(cref, 0);
}
static void call_on_write_cb(DemoConnectionRef cref, int status)
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    RBL_ASSERT((cref->on_write_cb != NULL), "write call back is NULL");
    RBL_ASSERT((cref->on_write_cb_arg != NULL), "write call arg back is NULL");
    void(*tmp)(void*, int) = cref->on_write_cb;
    void* arg = cref->on_write_cb_arg;
    cref->on_write_cb = NULL;
    cref->on_write_cb_arg = NULL;
    tmp(arg, status);
}
/////////////////////////////////////////////////////////////////////////////////////
// end of read sequence
/////////////////////////////////////////////////////////////////////////////////////
// Error functions
/////////////////////////////////////////////////////////////////////////////////////

static void write_error(DemoConnectionRef cref, char* msg)
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    printf("Write_error got an error this is the message: %s  fd: %d\n", msg, cref->asio_stream_ref->fd);
    cref->write_state = WRITE_STATE_STOP;
    call_on_write_cb(cref, DemoConnectionErrCode_io_error);
    post_to_runloop(cref, &postable_cleanup);
}
static void read_error(DemoConnectionRef cref, char* msg)
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    RBL_LOG_FMT("Read_error got an error this is the message: %s  fd: %d\n", msg, cref->asio_stream_ref->fd);
    cref->read_state = READ_STATE_STOP;
    RunloopRef rl = asio_stream_get_runloop(cref->asio_stream_ref);
    postable_cleanup(rl, cref);
}
/////////////////////////////////////////////////////////////////////////////////////
// end of error functions
/////////////////////////////////////////////////////////////////////////////////////
// cleanup sequence - functions called when connection is terminating
/////////////////////////////////////////////////////////////////////////////////////

/**
 * This must be the last democonnection function to run and it should only run once.
 */
static void postable_cleanup(RunloopRef runloop, void* arg_cref)
{
    DemoConnectionRef cref = arg_cref;
    RBL_LOG_FMT("postable_cleanup entered\n");
    RBL_ASSERT((cref->cleanup_done_flag == false), "cleanup should not run more than once");
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    asio_stream_close(cref->asio_stream_ref);
    cref->on_close_cb(cref->on_close_cb_arg);
}
/////////////////////////////////////////////////////////////////////////////////////
// end of cleanup
/////////////////////////////////////////////////////////////////////////////////////
