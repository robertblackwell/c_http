#define _GNU_SOURCE
#include <c_eg/worker.h>
#include <c_eg/constants.h>
#include <c_eg/alloc.h>
#include <c_eg/utils.h>
#include <c_eg/socket_functions.h>
#include <c_eg/queue.h>
#include <c_eg/parser.h>
#include <c_eg/message_reader.h>
#include <c_eg/message_writer.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include <pthread.h>

void socket_read(socket_handle_t socket, void* buffer, int buffer_length)
{
    int how_much = (int)read(socket, buffer, buffer_length);
    
    int saved_errno = errno;
    char* b = (char*)buffer;
    b[how_much] = '\0';
    printf("read socket buffer: \n%s\n", b);

    assert(how_much > 0);
    
}

void socket_write(socket_handle_t socket,  void* buffer, int buffer_length)
{
    int res = (int)write(socket, buffer, buffer_length);
    printf("socket_write res: %d\n", res);
    assert(res >= 0);
    assert(res == buffer_length); // need to handle partial write   
}

void write_string(int socket, char* msg)
{
    socket_write(socket, (void*) msg, strlen(msg));
}

void write_simple_response(socket_handle_t socket)
{
    const char* body = "<html><body><h2>This is a response</h2></body></html>";
    int len = strlen(body);
    char* msg;
    asprintf(&msg, "HTTP/1.1 200 OK\r\nContent-type: text/html\r\nContent-length: %d\r\n\r\n%s", len, body);
    printf("response\n%s", msg);
    write_string(socket, msg);
}

void http_handler(socket_handle_t socket)
{
    const int buffer_len = 10000;
    char buffer[buffer_len];
    {
        printf("Inside %s  socket: %d\n", __FUNCTION__, socket);
        socket_read(socket, buffer, buffer_len);
        write_simple_response(socket);
        sleep(5);
    }
}

struct Worker_s {
    bool        active;
    int         active_socket;
    QueueRef    qref;
    pthread_t   pthread;
    int         id;
};

WorkerRef Worker_new(QueueRef qref, int _id)
{
    WorkerRef wref = (WorkerRef)eg_alloc(sizeof(Worker));
    if(wref == NULL)
        return NULL;
    wref->active_socket = 0;
    wref->active  = false;
    wref->id = _id;
    wref->qref = qref;
    printf("returning from Worker_new\n");
    return wref;
}
void Worker_free(WorkerRef wref)
{
}
char* simple_response_body(char* message)
{

    char* body = "<html>"
                 "<head>"
                 "</head>"
                 "<body>"
                 "%s"
                 "</body>"
                 "</html>";

    char* s1;
    int len1 = asprintf(&s1, body, message);

    return s1;
}

char* simple_response(char* message)
{

    char* body = "<html>"
                 "<head>"
                 "</head>"
                 "<body>"
                 "%s"
                 "</body>"
                 "</html>";

    char* s1;
    int len1 = asprintf(&s1, body, message);

    char* prefix =  "HTTP/1.1 200 OK\r\n"
                    "content-length: %d\r\n"
                    "\r\n%s";
    char* s2;
    int len2 = asprintf(&s2, prefix, len1, s1);
    free(s1);
    return s2;
}
/**
 * Make a generic response consisting of 200 status, content length header and full request as body
 * @param request
 */
void make_generic_response(MessageRef request)
{
}
int handle_request(MessageRef request, MessageWriterRef wrtr)
{
    printf("Handle request\n");
    if (0) {
        /// would usually want to parse the target and dispatch based on the result
        CBufferRef target = Message_get_target(request);
        char* target_cstr = (char*)CBuffer_data(target);
        int target_len = CBuffer_size(target);
        struct http_parser_url u;
        http_parser_url_init(&u);
        int urc = http_parser_parse_url(target_cstr, target_len, 0, &u);
    }
    char* msg = "<h2>this is a message</h2>";
    char* body = simple_response_body(msg);
    int body_len = strlen(body);
    char* body_len_str;
    asprintf(&body_len_str, "%d", body_len);
    HeaderLineRef hl_content_length = HeaderLine_new(HEADER_CONTENT_LENGTH, strlen(HEADER_CONTENT_LENGTH), body_len_str, strlen(body_len_str));
    HDRListRef hdrs = HDRList_new();
    HDRList_add_front(hdrs, hl_content_length);

    MessageWriter_start(wrtr, HTTP_STATUS_OK, hdrs);
    MessageWriter_write_chunk(wrtr, (void*) body, body_len);
//    CBufferRef first_line = CBuffer_from_cstring("Http/1.1 200 OK\r\n");
//    CBufferRef serial = Message_serialize(request);
//    int length = CBuffer_size(serial);
//    char* s;
//    int len = asprintf(&s,"Content-length: %d\r\n", length);
//    CBuffer_append_cstr(first_line, s);
//    Buffer
    return 0;
}
static void* Worker_main(void* data)
{
    ASSERT_NOT_NULL(data);
    WorkerRef wref = (WorkerRef)data;
    while(true)
    {
        printf("Worker_main start of loop\n");
        wref->active = false;

        int mySocketHandle = Queue_remove(wref->qref);
        wref->active_socket = (int) mySocketHandle;
        int socket = mySocketHandle;
        wref->active = true;
        // in here read and service a http request
//        http_handler(wref->active_socket);
//        Worker_handler(wref);
        {
            ParserRef parser_ref = Parser_new();
            MessageReaderRef rdr = MessageReader_new(parser_ref, socket);
            MessageWriterRef wrtr = MessageWriter_new(socket);
            MessageRef request_msg_ref;
            MessageRef response_msg_ref;
            while((request_msg_ref = MessageReader_read(rdr)) != NULL) {
                printf("Got a request\n");
                handle_request(request_msg_ref, wrtr);
                Message_free(&request_msg_ref);
            }
            MessageWriter_free(&wrtr);
            MessageReader_free(&rdr);
        }
        wref->active = false;
    }
    return NULL;
}
// start a pthread - returns 0 on success errno on fila
void Worker_start(WorkerRef wref)
{
    ASSERT_NOT_NULL(wref);

    printf("Worker start - enter\n");
    int rc = pthread_create(&(wref->pthread), NULL, &(Worker_main), (void*) wref);
    printf("Worker start exit\n");
}
// void Worker_set_pthread(Workerref wref, pthread_t* pthread)
// {
//     wref->pthread = pthread;
// }
pthread_t* Worker_pthread(WorkerRef wref)
{
    ASSERT_NOT_NULL(wref);
    return &(wref->pthread);
}

