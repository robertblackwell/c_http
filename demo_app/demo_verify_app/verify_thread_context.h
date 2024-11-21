//
// Created by robert on 11/10/24.
//

#ifndef C_HTTP_Demo_VERIFY_THREAD_CONTEXT_H
#define C_HTTP_Demo_VERIFY_THREAD_CONTEXT_H
#include <http_in_c/common/verify_statistics.h>
#include <http_in_c/demo_protocol/demo_message.h>

struct VerifyThreadContext_s {
    /**
     * How many round trips in this experiment
     */
    int max_rountrips_per_connection;
    int rountrips_count;

    int max_connections_per_thread;
    /**
     * Unique identifier for this thread
     */
    int ident;
    /**
     * count of roundtrips completed
     */
    int roundtrip_per_connection_counter;
    int total_roundtrips;

    ResponseTimesArrayRef response_times_ref;
    /**
     * Total time for this thread
     */
//    double total_time;
    /**
     * response time for each round trip executed by this thread
     */
//    double resp_times[MAX_ROUNDTRIPS_PER_THREAD];

    /**
     * The most recent unique id string generated for round trip
     */
    char uid[100];
};

typedef struct VerifyThreadContext_s VerifyThreadContext, *VerifyThreadContextRef;

VerifyThreadContext* Ctx_new(int id, int max_roundtrips, int max_connections_per_thread, int max_threads);
DemoMessageRef mk_request(VerifyThreadContext* ctx);
void Ctx_mk_uid(VerifyThreadContext* ctx);
bool verify_response(VerifyThreadContext* ctx, DemoMessageRef request, DemoMessageRef response);

#endif //C_HTTP_DEMO_VERIFY_THREAD_CONTEXT_H
