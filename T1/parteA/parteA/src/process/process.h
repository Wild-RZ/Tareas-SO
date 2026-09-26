#pragma once
#include <stdbool.h>

typedef enum
{
    READY,
    WAITING,
    RUNNING,
    FINISHED,
    DEAD
} ProcessState;

typedef struct process
{

    char *name;
    int pid;

    // Datos que provienen del INPUT
    unsigned int t_inicio;
    unsigned int t_cpu_burst;
    unsigned int n_bursts;
    unsigned int io_wait;
    unsigned int t_deadline;

    // Para la simulación
    ProcessState state;
    unsigned int bursts_remaining;  // parte en n_bursts y disminuye al completar una ráfaga
    unsigned int burst_progress;    // ticks ya ejecutados en la ráfaga actual (se conserva al agotar el quantum)
    unsigned int quantum_remaining; // se reinicia al quantum completo en cada dispatch
    unsigned int io_remaining;      // cuenta regresiva mientras está en WAITING
    unsigned int t_ready;           // tick en que pasó a READY más recientemente, para desempate EDF

    // Metricas
    bool has_been_dispatched;
    int first_dispatch_time;      // -1 si nunca fue despachado
    unsigned int t_fin;           // tick en que llegó a FINISHED o DEAD
    unsigned int n_interruptions; // cuenta cada vez que agota su quantum sin terminar su ráfaga
    unsigned int ticks_running;   // total de ticks efectivamente en estado RUNNING
} Process;

Process process_create(const char *name, int pid, unsigned int t_inicio, unsigned int t_cpu_burst,
                       unsigned int n_bursts, unsigned int io_wait, unsigned int t_deadline);
void process_destroy(Process *p);

void process_set_ready(Process *p, unsigned int tick);