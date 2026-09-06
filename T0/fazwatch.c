#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include "protocol.h"
#include "fazwatch.h"

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
    printf("%s", argv[1]);
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

    int shutting_down = 0;

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
                        // 1. Buscar un slot disponible
                        int slot = -1;
                        for (int i = 0; i < MAX_CAMERAS; i++) {
                            if (cameras[i].state == EMPTY || cameras[i].state == FINISHED) {
                                slot = i;
                                break;
                            }
                        }

                        if (slot == -1) {
                            printf("Error: Limite maximo de camaras (%d) alcanzado.\n", MAX_CAMERAS);
                        } else {
                            int pipefd[2];
                            if (pipe(pipefd) == -1) {
                                perror("Error en pipe");
                                continue;
                            }

                            pid_t pid = fork();
                            if (pid == -1) {
                                perror("Error en fork");
                                close(pipefd[0]);
                                close(pipefd[1]);
                            } else if (pid == 0) {
                                // --- HIJO (CAMARA) ---
                                close(pipefd[0]); // Cerrar lectura

                                // Redirigir extremo de escritura al descriptor reservado (FD 3)
                                if (dup2(pipefd[1], FAZWATCH_FD) == -1) {
                                    perror("Error en dup2");
                                    exit(EXIT_FAILURE);
                                }
                                close(pipefd[1]); // Cerrar copia original

                                // Cerrar pipes heredados de otras camaras activas
                                for (int j = 0; j < MAX_CAMERAS; j++) {
                                    if (cameras[j].state != EMPTY && cameras[j].fd != -1) {
                                        close(cameras[j].fd);
                                    }
                                }

                                execvp(args[1], &args[1]);
                                perror("Error en execvp");
                                exit(EXIT_FAILURE);
                            } else {
                                // --- PADRE (FAZWATCH) ---
                                close(pipefd[1]);

                                cameras[slot].pid = pid;
                                cameras[slot].fd = pipefd[0];
                                strncpy(cameras[slot].name, args[1], sizeof(cameras[slot].name) - 1);
                                cameras[slot].state = RUNNING;
                                cameras[slot].last_heartbeat = time(NULL);
                                cameras[slot].exit_status = -1;

                                // Mapear al arreglo de poll (indice slot + 1)
                                fds[slot + 1].fd = pipefd[0];
                                fds[slot + 1].events = POLLIN;

                                printf("Camara iniciada con PID %d.\n", pid);
                            }
                        }
                    }
                }
                else if (strcmp(args[0], "status") == 0) {
                    printf("Ejecutando status...\n");
                    printf("PID\t| Ejecutable\t| Estado\t| Ultimo HB (s)\t| Exit Code/Signal\n");
    for (int i = 0; i < MAX_CAMERAS; i++) {
        if (cameras[i].state != EMPTY) {
            const char *state_str = "UNKNOWN";
            if (cameras[i].state == RUNNING) state_str = "RUNNING";
            else if (cameras[i].state == PAUSED) state_str = "PAUSED";
            else if (cameras[i].state == TERMINATING) state_str = "TERMINATING";
            else if (cameras[i].state == FINISHED) state_str = "FINISHED";

            long diff = (cameras[i].state == RUNNING) ? (time(NULL) - cameras[i].last_heartbeat) : 0;
            printf("%d\t| %s\t| %s\t| %ld\t\t| %d\n", 
                   cameras[i].pid, cameras[i].name, state_str, diff, cameras[i].exit_status);
        }
    }
}
                } 
                else if (strcmp(args[0], "pause") == 0) {
                    if (arg_count < 2) { 
                        printf("Error: pause requiere un PID.\n");
                    } else {
                        pid_t target_pid = atoi(args[1]);
                        for (int i = 0; i < MAX_CAMERAS; i++) {
                            if (cameras[i].pid == target_pid && cameras[i].state == RUNNING) {
                                kill(target_pid, SIGSTOP);
                                cameras[i].state = PAUSED;
                                printf("Camara %d pausada.\n", target_pid);
                                break;
                            }
                        }
                    }
                }
                else if (strcmp(args[0], "resume") == 0) {
                    if (arg_count < 2) {
                        printf("Error: resume requiere un PID.\n");
                    } else {
                        pid_t target_pid = atoi(args[1]);
                        for (int i = 0; i < MAX_CAMERAS; i++) {
                            if (cameras[i].pid == target_pid && cameras[i].state == PAUSED) {
                                kill(target_pid, SIGCONT);
                                cameras[i].state = RUNNING;
                                cameras[i].last_heartbeat = time(NULL); // Reiniciar timeout
                                printf("Camara %d reanudada.\n", target_pid);
                                break;
                            }
                        }
                    }
                }
                else if (strcmp(args[0], "terminate") == 0) {
                    if (arg_count < 2) {
                        printf("Error: terminate requiere un PID.\n");
                    } else {
                        pid_t target_pid = atoi(args[1]);
                        for (int i = 0; i < MAX_CAMERAS; i++) {
                            if (cameras[i].pid == target_pid && cameras[i].state == RUNNING) {
                                kill(target_pid, SIGTERM);
                                cameras[i].state = TERMINATING;
                                cameras[i].terminate_time = time(NULL);
                                printf("Terminando camara %d...\n", target_pid);
                                break;
                            }
                        }
                    }
                }
                else if (strcmp(args[0], "shutdown") == 0) {
                    printf("Iniciando apagado del sistema...\n");
                    shutting_down = 1;
                    for (int i = 0; i < MAX_CAMERAS; i++) {
                        if (cameras[i].state == RUNNING || cameras[i].state == PAUSED) {
                            kill(cameras[i].pid, SIGTERM);
                            cameras[i].state = TERMINATING;
                            cameras[i].terminate_time = time(NULL);
                        }
                    }
                } 
                else {
                    printf("Comando desconocido: %s\n", args[0]);
                }
            }
        } // Fin de if (fds[0].revents & POLLIN)

        // --- 1. LECTURA DE HEARTBEATS ---
        for (int i = 0; i < MAX_CAMERAS; i++) {
            if (cameras[i].state == RUNNING && (fds[i + 1].revents & POLLIN)) {
                char buffer[16];
                ssize_t bytes = read(cameras[i].fd, buffer, sizeof(buffer));
                if (bytes > 0) {
                    for (ssize_t b = 0; b < bytes; b++) {
                        if (buffer[b] == HEARTBEAT_BYTE) {
                            cameras[i].last_heartbeat = time(NULL);
                        }
                    }
                }
            }
        }
        
        // --- 2. TIMEOUTS Y PERIODO DE GRACIA ---
        time_t now = time(NULL);
        int all_finished = 1; 

        for (int i = 0; i < MAX_CAMERAS; i++) {
            if (cameras[i].state != EMPTY && cameras[i].state != FINISHED) {
                all_finished = 0; // Aún hay procesos vivos
            }

            // Detectar timeout en cámaras corriendo
            if (cameras[i].state == RUNNING && (now - cameras[i].last_heartbeat >= heartbeat_timeout)) {
                printf("Camara %d supero el timeout. Terminando...\n", cameras[i].pid);
                kill(cameras[i].pid, SIGTERM);
                cameras[i].state = TERMINATING;
                cameras[i].terminate_time = now;
            }
            
            // Aplicar SIGKILL tras 2 segundos de gracia
            if (cameras[i].state == TERMINATING && (now - cameras[i].terminate_time >= 2)) {
                kill(cameras[i].pid, SIGKILL);
            }
        }

        // --- 3. RECOLECCIÓN DE ZOMBIES Y DESCRIPTORES ---
        int status;
        pid_t dead_pid;
        while ((dead_pid = waitpid(-1, &status, WNOHANG)) > 0) {
            for (int i = 0; i < MAX_CAMERAS; i++) {
                if (cameras[i].pid == dead_pid && cameras[i].state != FINISHED) {
                    cameras[i].state = FINISHED;
                    
                    if (cameras[i].fd != -1) {
                        close(cameras[i].fd);
                        cameras[i].fd = -1;
                        fds[i + 1].fd = -1; // Desconectar de poll
                    }

                    if (WIFEXITED(status)) cameras[i].exit_status = WEXITSTATUS(status);
                    else if (WIFSIGNALED(status)) cameras[i].exit_status = WTERMSIG(status);
                    
                    printf("Camara %d recolectada.\n", dead_pid);
                    break;
                }
            }
        }

        // --- 4. APAGADO DEFINITIVO ---
        if (shutting_down && all_finished) {
            printf("Todas las camaras finalizadas. Cerrando fazwatch.\n");
            break; 
        }
    } // Fin del while(1)
    
    return EXIT_SUCCESS;
} // Fin del main