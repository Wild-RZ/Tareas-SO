#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../protocol.h"

static void send_heartbeat(void) {
    char heartbeat = HEARTBEAT_BYTE;

    if (write(FAZWATCH_FD, &heartbeat, sizeof(heartbeat)) == -1) {
        perror("camera_normal: write");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    int interval = 1;

    if (argc == 2) {
        interval = atoi(argv[1]);

        if (interval <= 0) {
            fprintf(stderr, "Uso: %s [intervalo_segundos]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    while (1) {
        send_heartbeat();
        sleep((unsigned int)interval);
    }
}
