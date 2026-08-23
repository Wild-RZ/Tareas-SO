#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../protocol.h"

static void send_heartbeat(void) {
    char heartbeat = HEARTBEAT_BYTE;

    if (write(FAZWATCH_FD, &heartbeat, sizeof(heartbeat)) == -1) {
        perror("camera_crash: write");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int heartbeat_count = 2;

    for (int i = 0; i < heartbeat_count; i++) {
        send_heartbeat();
        sleep(1);
    }

    raise(SIGSEGV);
    return EXIT_SUCCESS;
}
