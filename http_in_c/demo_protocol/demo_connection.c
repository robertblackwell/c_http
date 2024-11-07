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
        int socket,
        RunloopRef runloop_ref,
        DemoHandlerRef handler_ref,
        void(*connection_completion_cb)(void* href))
{
    DemoConnectionRef this = malloc(sizeof(DemoConnection));
    democonnection_init(this, socket, runloop_ref, handler_ref, connection_completion_cb);
    return this;
}
void democonnection_init(
        DemoConnectionRef this,
        int socket,
        RunloopRef runloop_ref,
        DemoHandlerRef handler_ref,
        void(*connection_completion_cb)(void* href))
{
    RBL_SET_TAG(DemoConnection_TAG, this)
    RBL_SET_END_TAG(DemoConnection_TAG, this)
    RBL_CHECK_TAG(DemoConnection_TAG, this)
    RBL_CHECK_END_TAG(DemoConnection_TAG, this)
    this->runloop_ref = runloop_ref;
    this->handler_ref = handler_ref;
    this->asio_stream_ref = asio_stream_new(runloop_ref, socket);
    this->read_buffer_size = 1000;
    this->active_input_buffer_ref = NULL;
    this->active_output_buffer_ref = NULL;
    this->read_state = READ_STATE_IDLE;
    this->write_state = WRITE_STATE_IDLE;
    this->readside_posted = false;
    this->writeside_posted = false;
    this->on_write_cb = NULL;
    this->on_read_cb = NULL;
    this->cleanup_done_flag = false;
    this->on_close_cb = connection_completion_cb;
    this->parser_ref = DemoParser_new(
            (void*)&on_read_complete,
            this);
}
void democonnection_free(DemoConnectionRef this)
{
    RBL_CHECK_TAG(DemoConnection_TAG, this)
    RBL_CHECK_END_TAG(DemoConnection_TAG, this)
    int fd = this->asio_stream_ref->fd;
    close(fd);
    asio_stream_free(this->asio_stream_ref);
    this->asio_stream_ref = NULL;
    DemoParser_dispose(&(this->parser_ref));
    if(this->active_output_buffer_ref) {
        IOBuffer_dispose(&(this->active_output_buffer_ref));
    }
    if(this->active_input_buffer_ref) {
        IOBuffer_dispose(&(this->active_input_buffer_ref));
    }
    free(this);
}
/////////////////////////////////////////////////////////////////////////////////////
// read sequence - sequence of functions called processing a read operation
////////////////////////////////////////////////////////////////////////////////////
void democonnection_read(DemoConnectionRef cref, void(*on_demo_read_cb)(void* href, DemoMessageRef, int statuc))
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    RBL_LOG_FMT("democonnect_read");
    assert(on_demo_read_cb != NULL);
    assert(cref->active_input_buffer_ref == NULL);
    read_start(cref);
}
static void read_start(DemoConnectionRef cref)
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
//    cref->readside_posted = true;
//    post_to_runloop(cref, &postable_reader);
    if (cref->active_input_buffer_ref == NULL) {
        cref->active_input_buffer_ref = IOBuffer_new_with_capacity(cref->read_buffer_size);
    }
    if(IOBuffer_data_len(cref->active_input_buffer_ref) == 0) {
        IOBufferRef iob = cref->active_input_buffer_ref;
        void *input_buffer_ptr = IOBuffer_space(iob);
        int input_buffer_length = IOBuffer_space_len(iob);
        asio_stream_read(cref->asio_stream_ref, input_buffer_ptr, input_buffer_length, &read_have_data_cb, cref);
    } else {
        read_process_data(cref);
    }
}
static void read_have_data_cb(void* cref_arg, long bytes_available, int err_status)
{
    DemoConnectionRef cref = cref_arg;
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    IOBufferRef iob = cref->active_input_buffer_ref;
    if(bytes_available > 0) {
        IOBuffer_commit(iob, bytes_available);
        read_process_data(cref);
    } else {
        read_error(cref, "");
    }
}
static void read_process_data(DemoConnectionRef cref) {
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    IOBufferRef iob = cref->active_input_buffer_ref;
    int bytes_available = IOBuffer_data_len(iob);
    if(bytes_available > 0) {
        RBL_LOG_FMT("Before DemoParser_consume read_state %d", cref->read_state);
        // TODO experiment with generic programming in C
        DemoParserRC ec = cref->parser_ref->parser_consume((ParserInterfaceRef)cref->parser_ref, iob);
//        int ec = DemoParser_consume(cref->parser_ref, iob);
        RBL_LOG_FMT("After DemoParser_consume returns  errno: %d read_state %d ", errno_save, cref->read_state);
        if(ec == DemoParserRC_message_incomplete) {
            // read more data
            post_to_runloop(cref, &postable_read_start);
        } else if(ec == DemoParserRC_end_of_message) {
            // have a complete message - parser will call the appropriate function
        } else {
            //parse error
            // parser will call the appropriate function
        }
    } else {
        // is this an error
        assert(false);
    }
    RBL_LOG_FMT("reader return\n");
}

static void postable_read_start(RunloopRef runloop_ref, void* arg)
{
    DemoConnectionRef cref = arg;
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    RBL_LOG_FMT("postable_read_start read_state: %d", cref->read_state);
    if(cref->read_state == READ_STATE_STOP) {
        cref->on_read_cb(cref->handler_ref, NULL, DemoConnectionErrCode_is_closed);
        return;
    }
    read_start(cref);
}

static void on_read_complete(DemoConnectionRef cref, DemoMessageRef msg, int error_code)
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    RBL_ASSERT( (((msg != NULL) && (error_code == 0)) || ((msg == NULL) && (error_code != 0))), "msg != NULL and error_code == 0 failed OR msg == NULL and error_code != 0 failed");
    cref->read_state = READ_STATE_IDLE;
    RBL_LOG_FMT("read_message_handler - on_write_cb  read_state: %d\n", cref->read_state);
    DC_Read_CB tmp = cref->on_read_cb;
    /**
     * Need the on_read_cb property NULL befor going further
     */
    cref->on_read_cb = NULL;
    tmp(cref->handler_ref, msg, error_code);
    /**
     * should either - close or throw away all input data
     * @TODO resolve
     */
}
/////////////////////////////////////////////////////////////////////////////////////
// end of read sequence
/////////////////////////////////////////////////////////////////////////////////////
// write sequence - sequence of functions called during write operation
////////////////////////////////////////////////////////////////////////////////////
void democonnection_write(
        DemoConnectionRef cref,
        DemoMessageRef response_ref,
        void(*on_demo_write_cb)(void* href, int statuc))
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    RBL_ASSERT((response_ref != NULL), "got NULL instead of a response_ref");
    RBL_ASSERT((on_demo_write_cb != NULL), "got NULL for on_demo_write_cb");
    RBL_ASSERT((cref->write_state == WRITE_STATE_IDLE), "a write is already active");
    RBL_ASSERT((cref->on_write_cb == NULL), "there is already a write in progress");
    RBL_ASSERT((cref->active_output_buffer_ref == NULL),"something wrong active output buffer should be null")
    cref->on_write_cb = on_demo_write_cb;
    cref->write_state == WRITE_STATE_ACTIVE;
    cref->active_output_buffer_ref = demo_message_serialize(response_ref);
    if(cref->write_state == WRITE_STATE_STOP) {
        cref->on_write_cb(cref->handler_ref, DemoConnectionErrCode_is_closed);
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
        IOBuffer_dispose(&(cref->active_output_buffer_ref));
        cref->on_write_cb(cref->handler_ref, DemoConnectionErrCode_is_closed);
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
    IOBuffer_consume(iob, bytes_written);
    if(IOBuffer_data_len(iob) > 0) {
        runloop_post(cref->runloop_ref, postable_writer, cref);
    } else {
        IOBuffer_dispose(&(cref->active_output_buffer_ref));
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
    RBL_ASSERT((cref->on_write_cb != NULL), "write call back is NULL");
    cref->on_write_cb(cref->handler_ref, 0);
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
    cref->on_write_cb(cref->handler_ref, DemoConnectionErrCode_io_error);
    cref->on_write_cb = NULL;
    post_to_runloop(cref, &postable_cleanup);
}
static void read_error(DemoConnectionRef cref, char* msg)
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    printf("Read_error got an error this is the message: %s  fd: %d\n", msg, cref->asio_stream_ref->fd);
    cref->read_state = READ_STATE_STOP;
    cref->on_read_cb(cref->handler_ref, NULL, DemoConnectionErrCode_io_error);
    cref->on_read_cb = NULL;
    post_to_runloop(cref, &postable_cleanup);
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
    printf("postable_cleanup entered\n");
    RBL_ASSERT((cref->cleanup_done_flag == false), "cleanup should not run more than once");
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    runloop_stream_deregister(cref->asio_stream_ref->runloop_stream_ref);
//    close(cref->socket_stream_ref->fd);
    cref->on_close_cb(cref->handler_ref);
}
/////////////////////////////////////////////////////////////////////////////////////
// end of cleanup
/////////////////////////////////////////////////////////////////////////////////////
// process request - DEPRECATED ... I think. Look in demohandler.c for
// these functions.
/////////////////////////////////////////////////////////////////////////////////////
static DemoMessageRef reply_invalid_request(DemoConnectionRef cref, const char* error_message)
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    DemoMessageRef m = demo_message_new();
    demo_message_set_is_request(m, false);
    BufferChainRef bdy =  BufferChain_new();
    BufferChain_append_cstr(bdy, "You made a mistake. message: ");
    char tmp[100];
    sprintf(tmp, "%s", error_message);
    BufferChain_append_cstr(bdy, tmp);
    demo_message_set_body(m, bdy);
    return m;
}
static DemoMessageRef process_request(DemoConnectionRef cref, DemoMessageRef request)
{
    RBL_CHECK_TAG(DemoConnection_TAG, cref)
    RBL_CHECK_END_TAG(DemoConnection_TAG, cref)
    DemoMessageRef reply = demo_message_new();
//    DemoMessageRef request = cref->parser_ref->m_current_message_ptr;
    demo_message_set_is_request(reply, false);
    BufferChainRef request_body = demo_message_get_body(request);
    BufferChainRef bc = BufferChain_new();
    BufferChain_append_bufferchain(bc, request_body);
    demo_message_set_body(reply, bc);
}