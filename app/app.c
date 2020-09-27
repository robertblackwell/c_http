#include <c_eg/server.h>
#include <c_eg/handler_example.h>
#include <stdio.h>
#include <mcheck.h>
#include<signal.h>

Server* g_sref;

void sig_handler(int signo)
{
    printf("app.c signal handler \n");
    if (signo == SIGINT) {
        printf("received SIGINT\n");
        Server_terminate( g_sref);
    }
}

int main()
{
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        printf("app.c main signal() failed");
    }
    printf("Hello this is main \n");
    Server* sref = Server_new(9001, handler_example);
    g_sref = sref;
    Server_listen(sref);
    Server_free(&sref);

}

