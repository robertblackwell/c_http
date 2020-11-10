#ifndef c_ceg_handler_h
#define c_ceg_handler_h

#include <c_http/constants.h>
#include <c_http/queue.h>
#include <c_http/worker.h>
#include <c_http/message.h>
#include <c_http/writer.h>
#include <c_http/socket_functions.h>
#include <c_http/handler_example.h>

/**
 * This is the signature of a handler function. *The address) of such a function must
 * be provided to Server_new() in order that the server and its worker threads
 * can call on this function to handle requests.
 *
 * The handler function is called once for each request and is passed the request message in its
 * entirety together with a Writer instance that provides functions to write the response.
 *
 * The handler function is solely responsible for constructing and sending the response.
 */
typedef int(*HandlerFunction)(MessageRef request, WriterRef wrttr);

#endif