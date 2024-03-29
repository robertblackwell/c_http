#include <http_in_c/demo_protocol/demo_client.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>
#include <math.h>

#define NBR_CONCURRENT 1
#define NBR_PER_THREAD 10

typedef struct ThreadContext {
    /**
     * How many round trips in this experiment
     */
    int howmany;
    /**
     * Unique identifier for this thread
     */
    int ident;
    /**
     * count of roundtrips completed
     */
    int counter;
    double total_time;
    double resp_times[NBR_PER_THREAD];
    /**
     * The most recent unique id string generated for round trip
     */
    char uid[100];
}ThreadContext;

ThreadContext* Ctx_new(int id)
{
    ThreadContext* ctx = malloc(sizeof(ThreadContext));
    ctx->howmany = NBR_PER_THREAD;
    ctx->ident = id;
    ctx->counter = 0;
}
/**
 * Create a request message with target = /echo, an Echo_id header, empty body
 * \param ctx
 * \return DemoMessageRef with ownership
 */
DemoMessageRef mk_request(ThreadContext* ctx)
{
    DemoMessageRef request = demo_message_new();
    demo_message_set_is_request(request, true);
    BufferChainRef body = BufferChain_new();
    char buf[100];
    sprintf(buf, "%d %d 1234567890", ctx->ident, ctx->counter);
    BufferChain_append_cstr(body, buf);
    demo_message_set_body(request, body);
    return request;
}

void Ctx_mk_uid(ThreadContext* ctx)
{
    sprintf(ctx->uid, "%d:%d", ctx->ident, ctx->counter);
}

/**
 * Verify that the response is correct based on the ctx->uid and request values
 * \param ctx       ThreadContext*
 * \param request   DemoMessageRef
 * \param response  DemoMessageRef
 * \return bool
 */
bool verify_response(ThreadContext* ctx, DemoMessageRef request, DemoMessageRef response)
{
    BufferChainRef body = demo_message_get_body(response);
    IOBufferRef body_iob = BufferChain_compact(body);
    const char* cstr = IOBuffer_cstr(body_iob);
    return true;
//    CbufferRef req_bc = Message_serialize(request);
//    int x = strcmp(Cbuffer_cstr(body_bc), Cbuffer_cstr(req_bc));
//    if( x != 0) {
//        printf("Verify failed \n");
//        printf("Req     :  %s\n", Cbuffer_cstr(req_bc));
//        printf("Rsp body:  %s\n", Cbuffer_cstr(body_bc));
//    }
//    return (x == 0);
}
struct timeval get_time()
{
    struct timeval t;
    struct timezone tzp;
    gettimeofday(&t, &tzp);
    return t;
//    return t.tv_sec*1e3 + t.tv_usec*1e-3;
}
double time_diff_ms(struct timeval t1, struct timeval t2)
{
    double dif = (t1.tv_sec - t2.tv_sec) + (t1.tv_usec - t2.tv_usec) * 1e-6;
    return dif;
}
void* threadfn(void* data)
{
    ThreadContext* ctx = (ThreadContext*)data;
    struct timeval start_time = get_time();
    for(int i = 0; i < ctx->howmany; i++) {
        struct timeval iter_start_time = get_time();
        DemoMessageRef response;
        DemoClientRef client = democlient_new();
        democlient_connect(client, "localhost", 9011);
        Ctx_mk_uid(ctx);
        DemoMessageRef request = mk_request(ctx);
        IOBufferRef serialized = demo_message_serialize(request);
        const char* req_buffer[] = {
            IOBuffer_cstr(serialized), NULL
        };

        democlient_roundtrip(client, req_buffer,  &response);
        IOBufferRef iob = demo_message_serialize(response);

        if(! verify_response(ctx, request, response)) {
            printf("Verify response failed");
        }
        ctx->counter++;
        IOBuffer_destroy(serialized);
        demo_message_dispose(&request);
        demo_message_dispose(&response);
        democlient_dispose(&client);
        struct timeval iter_end_time = get_time();
        ctx->resp_times[i] =  time_diff_ms(iter_end_time, iter_start_time);
    }
    struct timeval end_time = get_time();
    ctx->total_time =  time_diff_ms(end_time, start_time);
}

void combine_response_times(double all[NBR_PER_THREAD*NBR_CONCURRENT], double rt[NBR_PER_THREAD], int thrd_ix)
{
    for(int i = 0; i < NBR_PER_THREAD; i++) {
        double v = rt[i];
        all[thrd_ix*NBR_PER_THREAD + i] = rt[i];
    }
}
void stat_analyse(double all[NBR_PER_THREAD*NBR_CONCURRENT], double* average, double* stdev)
{
    double mean = 0.0;
    double total = 0.0;
    for(int i = 0; i < NBR_PER_THREAD*NBR_CONCURRENT; i++) {
        double v = all[i];
        total = total+v;
    }
    mean = total / (NBR_PER_THREAD*NBR_CONCURRENT*1.0);
    total = 0.0;
    for(int i = 0; i < NBR_PER_THREAD*NBR_CONCURRENT; i++) {
        double v = all[i];
        total = total+(v-mean)*(v-mean);
    }
    double variance = total / (NBR_PER_THREAD*NBR_CONCURRENT*1.0);
    double stddev = sqrt(variance);
    *average = mean;
    *stdev = stddev;
}

void analyse_response_times(double all[NBR_PER_THREAD*NBR_CONCURRENT], double buckets[10])
{
    double min = all[0];
    double max = 0.0;
    for(int i = 0; i < NBR_PER_THREAD*NBR_CONCURRENT; i++) {
        double v = all[i];
        if (min > v) min = v;
        if (max < v) max = v;
    }
    double bucket_gap = (max-min)/10.0;
    double bucket_lower[10];
    int bucket_count[10];
    for(int b = 0; b < 10; b++) {
        bucket_count[b] = 0;
        bucket_lower[b] = min+b*bucket_gap;
    }
    for(int i = 0; i <  NBR_PER_THREAD*NBR_CONCURRENT; i++) {
        for(int b = 0; b < 10; b++) {
            double z = min + (i*bucket_gap);
            if(all[i] >= min + i * bucket_gap) {
                bucket_count[b]++;
                break;
            }
        }
    }
    printf("Hello");
}

int main()
{
    int x1 = sizeof(char);
    int x2 = sizeof(char*);
    int x3 = sizeof(void*);
    int x4 = sizeof(int);
    int x5 = sizeof(long);
    int x6 = sizeof(long long);

    double all[NBR_CONCURRENT*NBR_PER_THREAD];
    struct timeval main_time_start = get_time();
    pthread_t workers[NBR_CONCURRENT];
    ThreadContext* tctx[NBR_CONCURRENT];
    for(int t = 0; t < NBR_CONCURRENT; t++) {
        ThreadContext* ctx = Ctx_new(t);
        tctx[t] = ctx;
        pthread_create(&(workers[t]), NULL, threadfn, (void*)ctx);
    }
    double tot_time = 0;
    for(int t = 0; t < NBR_CONCURRENT; t++) {
        pthread_join(workers[t], NULL);
        tot_time = tot_time + tctx[t]->total_time;
        combine_response_times(all, tctx[t]->resp_times, t);
    }
    int buckets[10];
    double avg;
    double stddev;
    stat_analyse(all, &avg, &stddev);
    struct timeval main_end_time = get_time();
    double main_elapsed = time_diff_ms(main_end_time, main_time_start);
    double av_time = main_elapsed / (NBR_CONCURRENT * 1.0);
    printf("Total elapsed time %f  threads: %d per thread: %d\n", main_elapsed, NBR_CONCURRENT, NBR_PER_THREAD);
    printf("Nbr threads : %d  nbr of requests per thread: %d av time %f \n", NBR_CONCURRENT, NBR_PER_THREAD, av_time);
    printf("Response times mean: %f stddev: %f\n", avg, stddev);

}

