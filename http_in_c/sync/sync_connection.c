
#define CHLOG_ON
#include <http_in_c/macros.h>
#include <http_in_c/sync/sync.h>
#include <http_in_c/socket_functions.h>
#include <http_in_c/sync/sync_internal.h>
#include <http_in_c/common/alloc.h>
#include <http_in_c/common/utils.h>
#include <http_in_c/common/iobuffer.h>
#include <http_in_c/http/parser.h>
#include <http_in_c/logger.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#define sync_connection_TAG "SYNCCONN"
#include <http_in_c/check_tag.h>

static llhttp_errno_t parser_on_message_handler(http_parser_t* parser_ptr, MessageRef input_message_ref)
{
    sync_connection_t* connptr = parser_ptr->handler_context;
    CHTTP_ASSERT((input_message_ref != NULL),"obvious");
    List_add_back(connptr->input_list, input_message_ref);
    connptr->reader_status = 1;
    return HPE_OK;
//    if(connptr->is_server_callback) {
//        connptr->input_message_ref = input_message_ref;
//        List_add_back(connptr->input_list, input_message_ref);
//        connptr->reader_status = 1;
//        sync_worker_t* workert_ptr = connptr->callback.server_cb.worker_ptr;
//        llhttp_errno_t r = connptr->callback.server_cb.server_handler(input_message_ref, workert_ptr);
//        return r;
//    } else {
//        connptr->input_message_ref = input_message_ref;
//        connptr->reader_status = 1;
//        List_add_back(connptr->input_list, input_message_ref);
//        sync_client_t* client_ptr = connptr->callback.client_cb.client_ptr;
//        llhttp_errno_t  r = connptr->callback.client_cb.client_handler(input_message_ref, client_ptr);
//        return r;
//    }
}
static void message_dealloc(void** p)
{
    void* mref = *p;
    Message_dispose(*p);
    *p = NULL;
}
void sync_connection_init(sync_connection_t* this, int socketfd, size_t read_buffer_size) //, SyncConnectionServerMessageHandler handler, sync_worker_r worker_ref)
{
    ASSERT_NOT_NULL(this);
    SET_TAG(SYNC_CONNECTION_TAG, this)
    LOG_FMT("sync_connection_init socketfd: %d", socketfd);
    this->m_parser = http_parser_new(&parser_on_message_handler, this);
    this->socketfd = socketfd;
//    this->m_rdsocket = rdsock;
    this->m_iobuffer = IOBuffer_new();
    this->input_list = List_new(message_dealloc);
    this->reader_status = 0;
    this->read_buffer_size = read_buffer_size;
//    this->callback.server_cb.server_handler = NULL;
//    this->callback.server_cb.worker_ptr = NULL;
}

sync_connection_t* sync_connection_new(int socketfd, size_t read_buffer_size) //, SyncConnectionServerMessageHandler handler, sync_worker_r worker_ref)
{
    sync_connection_t* rdr = malloc(sizeof(sync_connection_t));
    if(rdr == NULL)
        return NULL;
    sync_connection_init(rdr, socketfd, read_buffer_size); //, handler, worker_ref);
    return rdr;
}

void sync_connection_destroy(sync_connection_t* this)
{
    CHECK_TAG(SYNC_CONNECTION_TAG, this)
    IOBuffer_dispose(&(this->m_iobuffer));
    INVALIDATE_TAG(this)
    // INVALIDATE_STRUCT(this, sync_connection_t)
}
void sync_connection_dispose(sync_connection_t** this_ptr)
{
    sync_connection_t* this = *this_ptr;
    CHECK_TAG(SYNC_CONNECTION_TAG, this)
    sync_connection_destroy(this);
    eg_free((void*)this);
    *this_ptr = NULL;
}
/**
 * this function is private
 * Reads until -
 * -    the on_message_callback signals one or more messages have arrived
 *      Its signals this by setting this->status == 1 and this->input_message_ref != NULL.
 *      and
 *      The buffer that completed the input message has been consumed by the parser
 *
 *      In this situation the function returns HPE_OK
 *
 * -    some kind of io error or parse error. In which case the function returns
 *      HPE_?? but not HPE_OK
 *
 * To the reader - I am sorry this is really arcane
 *
 */
int sync_connection_read(sync_connection_t* this)
{
    CHECK_TAG(SYNC_CONNECTION_TAG, this)
    llhttp_errno_t rc;
    http_parser_t* pref = this->m_parser;
    while(1) {
        char buffer[1000000];
        char* b = (char*) &buffer;
        int length = 999000;
        LOG_FMT("sync_connection_read %p before read socket: %d", this, this->socketfd);
        socket_set_recvtimeout(this->socketfd, 5);
        LOGFMT("sync_connection_read before read from socket: %d ", this->socketfd);
        int nread = read(this->socketfd, (void*)b, length);
        int errno_saved = errno;
        LOGFMT("sync_connection_read after read from socket: %d nread: %d", this->socketfd, nread);
        if(nread > 0) {
            rc = http_parser_consume(pref, (void *) buffer, nread);
            if(this->socketfd == -1) {
                // the message handler function closed the socket
                return HPE_USER;
            }
            CHTTP_ASSERT((this->socketfd != -1), "fd closed under neath me");
            if(rc != HPE_OK) {
                if(rc == HPE_PAUSED) {
                    void* last_ptr = (void*)http_parser_last_byte_parsed(this->m_parser);
                    void* y = (last_ptr);
                    void* z = ((void*)buffer + (nread-1));
                    bool x = (last_ptr == ((void*)buffer + (nread-1)));
                    if(!x) {
                        LOG_ERROR("data left over after parsing last_ptr : %p  buffer+nread: %p", y, z);
                    }
                }
                return rc;
            } else {
                if(this->reader_status) {
                    CHTTP_ASSERT((List_size(this->input_list) > 0),"checking");
                    return HPE_OK;
                } else {
                    // go back around the while loop
                    printf("hello dolly");
                }

            }
        } else if(nread == 0) {
//            rc = http_parser_consume(pref, NULL, 0);
            return HPE_USER;
        } else {
            return HPE_USER;
            break;
        }
    }
}
int sync_connection_read_message(sync_connection_t* this, MessageRef *msg_ptr)
{
    CHECK_TAG(SYNC_CONNECTION_TAG, this)
    if(List_size(this->input_list) <= 0) {
        this->reader_status = 0;
        llhttp_errno_t rc = sync_connection_read(this);
        /**
         * connection_read() should have only two possible outcomes:
         * 1. got one or more input messages and consumed the most recent buffer without error
         * 2. hit an io error or parse error
         */
        bool invariant_gotone = ((rc == HPE_OK) && (this->reader_status == 1) && (List_size(this->input_list) > 0));
        bool invariant_goterror = ((rc!= HPE_OK) && (this->reader_status == 0) && (List_size(this->input_list) <= 0));
        CHTTP_ASSERT(invariant_gotone || invariant_goterror, "invariants failed");

        if(invariant_goterror) {
            *msg_ptr = NULL;
            return -1;
        }
    }
    CHTTP_ASSERT((List_size(this->input_list) > 0),"");
    MessageRef mref = List_remove_first(this->input_list);
    *msg_ptr = mref;
    return 1;
}

int sync_connection_read_request(sync_connection_t* this, SyncConnectionServerMessageHandler handler, sync_worker_r worker_ptr)
{
    CHECK_TAG(SYNC_CONNECTION_TAG, this)
//    this->is_server_callback = false;
//    this->callback.client_cb.client_handler = handler;
//    this->callback.client_cb.client_ptr = client_ptr;
//    llhttp_errno_t r = sync_connection_read(this);
    return (int)0;
}

int sync_connection_write(sync_connection_t* this, MessageRef msg_ref)
{
    CHECK_TAG(SYNC_CONNECTION_TAG, this)
    IOBufferRef serialized = Message_serialize(msg_ref);
    int len = IOBuffer_data_len(serialized);
    int rc = write(this->socketfd, IOBuffer_data(serialized), IOBuffer_data_len(serialized));
    return rc;
}
void sync_connection_close(sync_connection_t* this)
{
    CHECK_TAG(SYNC_CONNECTION_TAG, this)
    LOG_FMT("sync_connection_close %p socketfd: %d", this, this->socketfd);
    close(this->socketfd);
    this->socketfd = -1;
}
