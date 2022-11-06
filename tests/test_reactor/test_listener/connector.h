#ifndef c_http_tests_test_reactor_connector_h
#define c_http_tests_test_reactor_connector_h
#define _GNU_SOURCE
#define XR_TRACE_ENABLE
#include <c_http/async/types.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <c_http/unittest.h>
#include <c_http/common/utils.h>
#include <c_http/socket_functions.h>
#include <c_http/sync/sync_client.h>
#include <c_http/runloop/reactor.h>
#include <c_http/runloop/watcher.h>
#include <c_http/runloop/w_timerfd.h>
#include <c_http/runloop/w_iofd.h>
#include <c_http/runloop/w_listener.h>
#include <c_http/runloop/w_eventfd.h>



typedef struct Connector {
    int count;
    int max_count;
    int listen_fd;
    // TestAsyncServerRef servers[2];
}Connector, *ConnectorRef;

void* connector_thread_func(void* arg);

#endif