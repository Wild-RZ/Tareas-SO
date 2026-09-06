#include <stdbool.h>



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
