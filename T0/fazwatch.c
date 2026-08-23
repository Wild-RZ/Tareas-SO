#include <stdio.h>
#include <stdlib.h>

#include "protocol.h"

int main(int argc, char **argv) {
    int heartbeat_timeout = 3;

    if (argc > 2) {
        fprintf(stderr, "Uso: %s [heartbeat_timeout]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        heartbeat_timeout = atoi(argv[1]);
    }

    printf("FAZWATCH iniciado. Timeout de heartbeat: %d segundos.\n",heartbeat_timeout);

    while (1){

        //TODO: Implementar el ciclo principal de fazwatch
    }

    return EXIT_SUCCESS;
}