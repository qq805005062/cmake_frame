#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "RedisAsync.h"

#if 0
void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = static_cast<redisReply *>(r);
    if (reply == NULL) return;
    printf("argv[%s]: %s\n", (char*)privdata, reply->str);

    /* Disconnect after receiving the reply to GET */
    redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}
#endif

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
#if 0
    struct event_base *base = event_base_new();

    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return 1;
    }

    redisLibeventAttach(c,base);
    redisAsyncSetConnectCallback(c,connectCallback);
    redisAsyncSetDisconnectCallback(c,disconnectCallback);
    redisAsyncCommand(c, NULL, NULL, "SET key %b", argv[argc-1], strlen(argv[argc-1]));
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET key");
    event_base_dispatch(base);
#endif
	ASYNCREDIS::RedisAsync hha;

	hha.RedisConnect("127.0.0.1", 6379,5);
	hha.RedisLoop();
    return 0;
}
