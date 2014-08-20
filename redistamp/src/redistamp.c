#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "hiredis.h"

#define STATUS_SECONDS 300

void main_loop();
int stamp_loop(redisContext *conn);
int check_replication_lag(redisContext *conn, time_t time_secs);
void mode_switch(int new_mode);

struct sockaddr_in statsd_addr;
int statsd_sock;

char *host;
int port = 6379;

#define MASTER_MODE 1
#define SLAVE_MODE 2
int mode = 0;

int main(int argc, char *argv[]) {
	char *badnum;

	/* All timestamps in UTC: */
	setenv("TZ", "UTC", 0);
	tzset();

	/* Parse args: */
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

	/* Set up statsd socket: */
	if ((statsd_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("socket");
		exit(1);
	}
	memset((char *) &statsd_addr, 0, sizeof(statsd_addr));
	statsd_addr.sin_family = AF_INET;
	statsd_addr.sin_port = htons(8125);
	if (inet_aton("127.0.0.1", &statsd_addr.sin_addr) <= 0) {
		perror("inet_aton");
		exit(1);
	}

	/* And go! */
	while (1) {
		main_loop();
		sleep(3);
	}
}

void main_loop() {
	struct timeval timeout = { 1, 0 }; // 1 second
	redisContext *conn;
	int counter = STATUS_SECONDS;
	int status;

	printf("Connecting to Redis on %s, port %d ... ", host, port);
	fflush(stdout);

	conn = redisConnectWithTimeout(host, port, timeout);

	if (conn->err) {
		printf("error: %s\n", conn->errstr);
		fflush(stdout);
		redisFree(conn);
		return;
	}

	printf("connected.\n");
	fflush(stdout);

	while ((status = stamp_loop(conn))) {
		if (status < 0) {
			counter = STATUS_SECONDS;
		} else {
			counter -= 1;
			if (counter <= 0) {
				printf("Happily %s %s.\n", mode == MASTER_MODE ? "stamping" : "monitoring", host);
				counter = STATUS_SECONDS;
			}
		}

		fflush(stdout);
		sleep(1);
	}

	printf("Lost connection: %s\n", conn->err ? conn->errstr : "(unknown cause)");
	fflush(stdout);
	redisFree(conn);
}

int stamp_loop(redisContext *conn) {
	redisReply *reply;
	time_t raw_time;
	struct tm *utc_time;
	char timestamp[45];
	int status = -1;

	raw_time = time(NULL);
	utc_time = localtime(&raw_time);
	strftime(timestamp, sizeof(timestamp) - 1, "%s: %Y-%m-%d %H:%M:%S %Z", utc_time);

	reply = redisCommand(conn, "SET redistamp %s", timestamp);
	if (!reply) { return 0; }

	switch (reply->type) {
		case REDIS_REPLY_STATUS:
			if (!strncmp(reply->str, "OK", 3)) {
				mode_switch(MASTER_MODE);
				status = 1;
				break;
			}
			// otherwise error, fall through

		case REDIS_REPLY_ERROR:
			if (!strncmp("READONLY ", reply->str, 9)) {
				mode_switch(SLAVE_MODE);
				status = check_replication_lag(conn, raw_time);
			} else {
				printf("SET redistamp \"%s\": %s\n", timestamp, reply->str);
			}
			break;

		default:
			printf("SET redistamp \"%s\": unknown response type\n", timestamp);
			break;
	}

	freeReplyObject(reply);
	return status;
}

void mode_switch(int new_mode) {
	if (mode == new_mode)
		return;

	switch (new_mode) {
		case MASTER_MODE: printf("Now in master mode (stamping).\n"); break;
		case SLAVE_MODE:  printf("Now in slave mode (monitoring).\n"); break;
		default:
			printf("mode_switch: Unknown mode %d\n", mode);
			return;
	}

	mode = new_mode;
}

int check_replication_lag(redisContext *conn, time_t time_secs) {
	redisReply *reply;
	int status = -1;
	time_t timestamp;
	time_t lag;
	char buf[100];

	reply = redisCommand(conn, "GET redistamp");
	if (!reply) { return 0; }

	switch (reply->type) {
		case REDIS_REPLY_STRING:
			timestamp = atoi(reply->str);
			/* Ignore timestamps under 10k because they're probably "2014",
			 * before we added the raw timestamp to the start. */
			if (timestamp > 10000) {
				lag = time_secs - timestamp;
				snprintf(buf, sizeof(buf) - 1, "redis.redistamp.lag:%ld|g|#port=%d", lag, port);
				if (sendto(statsd_sock, buf, strlen(buf), 0, (struct sockaddr *) &statsd_addr, sizeof(statsd_addr)) < 0) {
					perror("sendto");
				}

				if (lag <= 1) {
					status = 1;
				} else {
					printf("Replication lag of %ld seconds detected!\n", lag);
				}
			} else {
				printf("GET redistamp: no timestamp\n");
			}
			break;

		case REDIS_REPLY_NIL:
			printf("GET redistamp: key not found\n");
			break;

		case REDIS_REPLY_INTEGER:
		case REDIS_REPLY_ARRAY:
			printf("GET redistamp: wrong datatype\n");
			break;

		case REDIS_REPLY_ERROR:
			printf("GET redistamp: %s\n", reply->str);
			break;

		default:
			printf("GET redistamp: unknown response type\n");
			break;
	}

	return status;
}
