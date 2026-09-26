#pragma once
#include "../process/process.h"

typedef struct queue
{
    Process **items; // arreglo de punteros a procesos activos (READY o WAITING)
    int size;
    int capacity;
} Queue;

void queue_init(Queue *q, int capacity);
void queue_free(Queue *q);

void queue_push(Queue *q, Process *p);
void queue_remove(Queue *q, Process *p);

Process *queue_pop_best_ready(Queue *q);
void queue_sort(Queue *q);

// Comparador de tres niveles (Sección 3 del enunciado):
// 1) menor t_deadline
// 2) en caso de empate, menor t_ready
// 3) en caso de empate también en t_ready, menor pid
// Retorna -1 si "a" tiene mayor prioridad que "b", 1 si es al revés, 0 si son indistinguibles.
int process_compare(const Process *a, const Process *b);