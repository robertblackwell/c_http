
#ifndef C_HTTP_MESSAGE_H
#define C_HTTP_MESSAGE_H
#include <stdbool.h>
#include <stdint.h>
#include <http_in_c/common/buffer_chain.h>
#include <http_in_c/http/parser_types.h>
#include <http_in_c/http/header_list.h>
/**
 * @addtogroup group_message
 * @brief A module that implements a http/1.1 message container
 * ## Module Message
 * This is some documentation for the messages modules.
 * -    not yet
 * @{
 */

struct Message_s;
typedef struct Message_s Message, *MessageRef;

enum HttpMajorVersion {major_version1=1};
enum HttpMinorVersion {minor_verson0=0, minor_version1=1};

#define HttpStatusString(status) http_status_str(status)
#define HttpMethodString(method) http_method_str(method)
#define HttpErrnoName(er) http_errno_name(er)
#define HttpErrnoDescription(er) http_errno_description(er)

#define HEADER_HOST "HOST"
#define HEADER_CONNECTION_KEY "CONNECTION"
#define HEADER_CONNECTION_CLOSE "close"
#define HEADER_CONNECTION_KEEPALIVE "keep-alive"
#define HEADER_PROXYCONNECTION "PROXY-CONNECTION"
#define HEADER_CONTENT_LENGTH "CONTENT-LENGTH"
#define HEADER_TRANSFERENCODING "TRANSFER-ENCODING"
#define HEADER_CONTENT_TYPE "CONTENT-TYPE"
#define HEADER_ECHO_ID "C-HTTP-ECHO-ID"


// an iterator for pointing at a header line. NULL means does not reference a header line
typedef void* HeaderIter;

typedef int HttpMinorVersion;
#define Message_TAG "MESSAGE"

MessageRef Message_new();
MessageRef Message_new_request();
MessageRef Message_new_response();

void Message_dispose(MessageRef* p);
void Message_dealloc(void* m);
/**
 * @brief Methods to test and set whether a message is a request or response
 */
bool Message_is_request(MessageRef mref);
bool Message_get_is_request(MessageRef this);
void Message_set_is_request(MessageRef this, bool yn);
/**
 * @brief Methods to get/set all first line fields
 */
HttpStatus Message_get_status(MessageRef mref);
void Message_set_status(MessageRef mref, HttpStatus status);

HttpMethod Message_get_method(MessageRef mref);
void Message_set_method(MessageRef mref, HttpMethod method);

HttpMinorVersion Message_get_minor_version(MessageRef mref);
void Message_set_minor_version(MessageRef this, HttpMinorVersion mv);
void Message_set_version(MessageRef this, int maj, int minor);
/**
 * The first line fields that have string values (such as reason and target) come in two version:.
 *  -   const char* - version that takes or returns const char* which are all c strings. For this style
 *      -    the getter returns a pointer to a char* owned by the Message instance, do not free or modify
 *      -    the setter version copies the const char* parameter so that no transfer of ownership of the char* value takes place
 *  - CbufferRef - version that take or return CbufferRef. These functions are intended mainly for proxy use where
 *    first line values will be taken from an existing Message instance. Also the first line string values are stored
 *    in the message instance as CbufferRef.
 *      -   the getter returns an ownership along with the BufferRef so the caller is responsible for freeing the returned instance
 *      -   the setter makes a copy of the CbufferRef parameter so ownershp stays with the caller.
 */
void Message_set_target(MessageRef this, const char* target_cstr);
const char* Message_get_target(MessageRef this);

void Message_set_target_cbuffer(MessageRef this, CbufferRef target);
CbufferRef Message_get_target_cbuffer(MessageRef this);

void Message_set_reason(MessageRef this, const char* reason_cstr);
const char* Message_get_reason(MessageRef this);

void Message_set_reason_cbuffer(MessageRef this, CbufferRef reason);
CbufferRef Message_get_reason_cbuffer(MessageRef this);

void Message_set_content_length(MessageRef this, int length);
/**
 * Add a new header line to the message. If the key is already in the header list
 * replace its value with the value provided in this call
 *
 * @param mref  MessageRef
 * @param label const char* The label or key for the header line to be added. It is a c-string
 *                          and the value is copied by this call. Always upper case
 * @param value const char* The value part of the header line to be added. This is a c-string
 *                          and the value is copied
 */
void Message_add_header_cstring(MessageRef mref, const char* label, const char* value);
void Message_add_header_cbuf(MessageRef this, CbufferRef key, CbufferRef value);

/**
 * Get the value string for the header line with the given label or key.
 * If not found return NULL
 * @param mref   MessageRef
 * @param labptr The key or label for the header line being sought
 * @return const char* or NULL  The return value(when not NULL) is a weak reference to
 *                              a string that is owned by the message instance. Do not free
 *                              value is always lower case. The weak ref becomes invalid
 *                              when the message is freed/disposed
 */
const char* Message_get_header_value(MessageRef mref, const char* labptr);
HdrListRef Message_get_headerlist(MessageRef this);
/**
 * Compares a header
 * @return
 *  -   1 if key is in the header list and has the test value (when case ignored)
 *  -   0 if key is in header list but test value is different to header value
 *  -   -1 key is not in header list
 */
int Message_cmp_header(MessageRef msgref, const char* key, const char* test_value);

void Message_set_headers_arr(MessageRef mref, const char* ar[][2]);

//const char* Message_header_iter_deref(MessageRef mref, HeaderIter iter);

/**
 * Within a MessageRef the body of the message is stored as either NULL or BufferChainref.
 * This function returns NULL or a BufferChainRef
 * @param mref MessageRef
 * @return BufferChainRef  WARNING: The return value when not NULL is a reference that is also
 *                          held by the Message instance. Be aware that there will be two
 *                          references to the same BufferChain.
 *                          The Message_dispose() function will dispose the buffer chain
 *                          Consider that the MessageRef retains ownership of the BufferChainRef
 */
BufferChainRef Message_get_body(MessageRef mref);
/**
 * Following on from the previous function doc block. This function gives ownership of a BufferChainRef to
 * a Message instance.
 * @param mref   MessageRef
 * @param body   BufferChainRef
 */
void Message_set_body(MessageRef mref, BufferChainRef bodyp);


IOBufferRef Message_serialize(MessageRef this);
IOBufferRef Message_dump(MessageRef this);

/**@}*/
#endif