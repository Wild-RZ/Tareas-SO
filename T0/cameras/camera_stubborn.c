#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../protocol.h"

static void send_heartbeat(void) {
    char heartbeat = HEARTBEAT_BYTE;

    if (write(FAZWATCH_FD, &heartbeat, sizeof(heartbeat)) == -1) {
        perror("camera_stubborn: write");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    int heartbeat_count = 3;

    if (argc == 2) {
        heartbeat_count = atoi(argv[1]);
    }

    if (heartbeat_count < 0 || argc > 2) {
        fprintf(stderr, "Uso: %s [cantidad_heartbeats]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (signal(SIGTERM, SIG_IGN) == SIG_ERR) {
        perror("camera_stubborn: signal");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < heartbeat_count; i++) {
        send_heartbeat();
        sleep(1);
    }

    while (1) {
        sleep(1);
    }
}
