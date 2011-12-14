#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "hiredis.h"

#define STATUS_SECONDS 300

void main_loop(char *host, int port);
int stamp_loop(redisContext *conn);

int main(int argc, char *argv[]) {
	char *host;
	int port = 6379;
	char *badnum;

	setenv("TZ", "UTC", 0);
	tzset();

	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage:\t%s <host> [port]\n\tPort defaults to %d.\n", argv[0], port);
		exit(1);
	}

	host = argv[1];
	if (argc == 3) {
		port = strtol(argv[2], &badnum, 10);
		if (*argv[2] == '\0' || *badnum != '\0') {
			fprintf(stderr, "Bad port: \"%s\"\n", argv[2]);
			exit(1);
		}
	}

	while (1) {
		main_loop(host, port);
		sleep(3);
	}
}

void main_loop(char *host, int port) {
	struct timeval timeout = { 1, 0 }; // 1 second
	redisContext *conn;
	int counter = STATUS_SECONDS;
	int status;

	printf("Connecting to Redis on %s, port %d ... ", host, port);

	conn = redisConnectWithTimeout(host, port, timeout);

	if (conn->err) {
		printf("error: %s\n", conn->errstr);
		redisFree(conn);
		return;
	}

	printf("connected.\n");

	while ((status = stamp_loop(conn))) {
		if (status < 0) {
			counter = STATUS_SECONDS;
		} else {
			counter -= 1;
			if (counter <= 0) {
				printf("Happily stamping %s.\n", host);
				counter = STATUS_SECONDS;
			}
		}

		sleep(1);
	}

	printf("Lost connection: %s\n", conn->err ? conn->errstr : "(unknown cause)");
	redisFree(conn);
}

int stamp_loop(redisContext *conn) {
	redisReply *reply;
	time_t raw_time;
	struct tm *utc_time;
	char timestamp[30];
	int status = -1;

	raw_time = time(NULL);
	utc_time = localtime(&raw_time);
	strftime(timestamp, sizeof(timestamp) - 1, "%Y-%m-%d %H:%M:%S %Z", utc_time);

	reply = redisCommand(conn, "SET redistamp %s", timestamp);
	if (!reply) { return 0; }

	switch (reply->type) {
		case REDIS_REPLY_STATUS:
			if (!strncmp(reply->str, "OK", 3)) {
				status = 1;
				break;
			}
			// otherwise error, fall through

		case REDIS_REPLY_ERROR:
			printf("SET redistamp \"%s\": %s\n", timestamp, reply->str);
			break;

		default:
			printf("SET redistamp \"%s\": unknown response type\n", timestamp);
			break;
	}

	freeReplyObject(reply);
	return status;
}

