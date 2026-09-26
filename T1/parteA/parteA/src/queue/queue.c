#include "queue.h"
#include <stdlib.h>

int process_compare(const Process *a, const Process *b)
{
    if (a->t_deadline < b->t_deadline)
        return -1;
    if (a->t_deadline > b->t_deadline)
        return 1;

    if (a->t_ready < b->t_ready)
        return -1;
    if (a->t_ready > b->t_ready)
        return 1;

    if (a->pid < b->pid)
        return -1;
    if (a->pid > b->pid)
        return 1;

    return 0;
}

void queue_init(Queue *q, int capacity)
{
    q->items = calloc(capacity, sizeof(Process *));
    q->size = 0;
    q->capacity = capacity;
}

void queue_free(Queue *q)
{
    free(q->items);
    q->items = NULL;
    q->size = 0;
    q->capacity = 0;
}

void queue_push(Queue *q, Process *p)
{
    if (q->size < q->capacity)
    {
        q->items[q->size] = p;
        q->size++;
    }
}

void queue_remove(Queue *q, Process *p)
{
    for (int i = 0; i < q->size; i++)
    {
        if (q->items[i] == p)
        {
            q->items[i] = q->items[q->size - 1];
            q->size--;
            return;
        }
    }
}

Process *queue_pop_best_ready(Queue *q)
{
    int best_index = -1;

    for (int i = 0; i < q->size; i++)
    {
        if (q->items[i]->state != READY)
            continue;

        if (best_index == -1 || process_compare(q->items[i], q->items[best_index]) < 0)
            best_index = i;
    }

    if (best_index == -1)
        return NULL;

    Process *chosen = q->items[best_index];
    q->items[best_index] = q->items[q->size - 1];
    q->size--;
    return chosen;
}

void queue_sort(Queue *q)
{
    for (int i = 1; i < q->size; i++)
    {
        Process *key = q->items[i];
        int j = i - 1;
        while (j >= 0 && process_compare(q->items[j], key) > 0)
        {
            q->items[j + 1] = q->items[j];
            j--;
        }
        q->items[j + 1] = key;
    }
}