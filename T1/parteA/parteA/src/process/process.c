#include "process.h"
#include <stdlib.h>
#include <string.h>

Process process_create(const char *name, int pid, unsigned int t_inicio, unsigned int t_cpu_burst,
                       unsigned int n_bursts, unsigned int io_wait, unsigned int t_deadline)
{
  Process p;

  p.name = strdup(name);
  p.pid = pid;

  // Datos que provienen del INPUT
  p.t_inicio = t_inicio;
  p.t_cpu_burst = t_cpu_burst;
  p.n_bursts = n_bursts;
  p.io_wait = io_wait;
  p.t_deadline = t_deadline;

  // Para la simulación
  p.state = READY;
  p.bursts_remaining = n_bursts;
  p.burst_progress = 0;
  p.quantum_remaining = 0;
  p.io_remaining = 0;
  p.t_ready = t_inicio;

  // Metricas
  p.has_been_dispatched = false;
  p.first_dispatch_time = -1;
  p.t_fin = 0;
  p.n_interruptions = 0;
  p.ticks_running = 0;

  return p;
}

void process_destroy(Process *p)
{
  free(p->name);
  p->name = NULL;
}

void process_set_ready(Process *p, unsigned int tick)
{
  p->state = READY;
  p->t_ready = tick;
}