#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include "protocol.h"

typedef enum {
    RUNNING,
    PAUSED,
    TERMINATING,
    FINISHED,
    EMPTY // Estado auxiliar para saber si el espacio está libre en nuestro arreglo
} CameraState;

typedef struct {
    pid_t pid;
    int fd;                     // El extremo de lectura del pipe
    char name[128];             // Nombre del ejecutable
    CameraState state;
    time_t last_heartbeat;      // Marca de tiempo del último heartbeat
    time_t terminate_time;      // Para los 2 segundos de gracia tras SIGTERM
    int exit_status;            // Código de salida
} Camera;

Camera cameras[MAX_CAMERAS];

int main(int argc, char **argv) {
    int heartbeat_timeout = 3;

    if (argc > 2) {
        fprintf(stderr, "Uso: %s [heartbeat_timeout]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 2) {
        heartbeat_timeout = atoi(argv[1]);
    }

    printf("FAZWATCH iniciado. Timeout de heartbeat: %d segundos.\n", heartbeat_timeout);

    // 1. Inicializamos las cámaras marcándolas como vacías
    for (int i = 0; i < MAX_CAMERAS; i++) {
        cameras[i].state = EMPTY;
    }

    // 2. Preparamos el arreglo para poll (MAX_CAMERAS + 1 espacio para la terminal)
    struct pollfd fds[MAX_CAMERAS + 1];
    
    // El primer elemento (índice 0) es la terminal (stdin)
    fds[0].fd = STDIN_FILENO; // Descriptor 0
    fds[0].events = POLLIN;   // Queremos saber si hay datos para leer

    // Inicializamos los fds de las cámaras con -1 (inactivos)
    for (int i = 1; i <= MAX_CAMERAS; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }

    char command_buffer[128]; // Los comandos tienen longitud máxima de 128 caracteres

    while (1){
        // 3. Llamamos a poll con un timeout corto (500 ms). 
        // Esto evita que se bloquee el ciclo principal.
        int poll_count = poll(fds, MAX_CAMERAS + 1, 500); 

        if (poll_count < 0) {
            perror("Error en poll");
            break;
        }

        // 4. Revisar si el usuario escribió un comando en la terminal
        if (fds[0].revents & POLLIN) {
            if (fgets(command_buffer, sizeof(command_buffer), stdin) != NULL) {
                // Quitar el salto de línea al final del comando
                command_buffer[strcspn(command_buffer, "\n")] = 0; 

                if (strlen(command_buffer) == 0) continue;

                // Arreglo de punteros para guardar cada palabra del comando
                char *args[32]; 
                int arg_count = 0;
                // strtok corta el string cada vez que encuentra un espacio " "
                char *token = strtok(command_buffer, " ");
                while (token != NULL && arg_count < 31) {
                    args[arg_count] = token;
                    arg_count++;
                    token = strtok(NULL, " ");
                }
                args[arg_count] = NULL; // El último elemento debe ser NULL para execvp

                // Parsear e identificar el comando
                if (strcmp(args[0], "launch") == 0) {
                    if (arg_count < 2) {
                        printf("Error: launch requiere al menos un ejecutable.\n");
                    } else {
                        printf("Iniciando launch para el ejecutable: %s\n", args[1]);
                        // Por hacer: Implementar la logica de fork y pipe para launch
                    }
                } 
                else if (strcmp(args[0], "status") == 0) {
                    printf("Ejecutando status...\n");
                    // Por hacer: Mostrar el estado de todas las camaras
                } 
                else if (strcmp(args[0], "pause") == 0) {
                    if (arg_count < 2) printf("Error: pause requiere un PID.\n");
                    else printf("Pausando PID: %s\n", args[1]);
                    // Por hacer: Implementar envio de SIGSTOP
                } 
                else if (strcmp(args[0], "resume") == 0) {
                    if (arg_count < 2) printf("Error: resume requiere un PID.\n");
                    else printf("Reanudando PID: %s\n", args[1]);
                    // Por hacer: Implementar envio de SIGCONT
                } 
                else if (strcmp(args[0], "terminate") == 0) {
                    if (arg_count < 2) printf("Error: terminate requiere un PID.\n");
                    else printf("Terminando PID: %s\n", args[1]);
                    // Por hacer: Implementar envio de SIGTERM
                } 
                else if (strcmp(args[0], "shutdown") == 0) {
                    printf("Iniciando apagado del sistema...\n");
                    // Por hacer: Implementar logica de shutdown
                } 
                else {
                    printf("Comando desconocido: %s\n", args[0]);
                }
            }
        }
            }
        }

        // Por hacer: Revisar los heartbeats de los hijos
        // Por hacer: Limpiar procesos zombies con waitpid
    }

    return EXIT_SUCCESS;
}