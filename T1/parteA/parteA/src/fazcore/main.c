#include <stdio.h>
#include <stdlib.h>
#include "../process/process.h"
#include "../queue/queue.h"

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		fprintf(stderr, "Uso: %s <input_file> <output_file>\n", argv[0]);
		return 1;
	}

	FILE *input_file = fopen(argv[1], "r");
	if (input_file == NULL)
	{
		fprintf(stderr, "No se pudo abrir el archivo de input: %s\n", argv[1]);
		return 1;
	}

	unsigned int q;
	int n_procesos;
	fscanf(input_file, "%u", &q);
	fscanf(input_file, "%d", &n_procesos);

	Process *procesos = malloc(n_procesos * sizeof(Process));

	for (int i = 0; i < n_procesos; i++)
	{
		char name_buffer[64];
		int pid;
		unsigned int t_inicio, t_cpu_burst, n_bursts, io_wait, t_deadline;

		fscanf(input_file, "%63s %d %u %u %u %u %u",
			   name_buffer, &pid, &t_inicio, &t_cpu_burst, &n_bursts, &io_wait, &t_deadline);

		procesos[i] = process_create(name_buffer, pid, t_inicio, t_cpu_burst, n_bursts, io_wait, t_deadline);
	}

	fclose(input_file);

	Queue queue;
	queue_init(&queue, n_procesos);

	Process *cpu = NULL;
	unsigned int tick = 0;
	int finished_count = 0;

	while (finished_count < n_procesos)
	{
		for (int i = 0; i < n_procesos; i++)
		{
			if (procesos[i].t_inicio == tick)
			{
				process_set_ready(&procesos[i], tick);
				queue_push(&queue, &procesos[i]);
			}
		}

		for (int i = 0; i < queue.size; i++)
		{
			Process *p = queue.items[i];
			if (p->state == WAITING)
			{
				p->io_remaining--;
				if (p->io_remaining == 0)
				{
					process_set_ready(p, tick);
				}
			}
		}

		int i = 0;
		while (i < queue.size)
		{
			Process *p = queue.items[i];
			if (tick >= p->t_deadline)
			{
				p->state = DEAD;
				p->t_fin = tick;
				finished_count++;
				queue_remove(&queue, p);
			}
			else
			{
				i++;
			}
		}

		if (cpu == NULL)
		{
			queue_sort(&queue);
		}

		if (cpu != NULL)
		{
			cpu->burst_progress++;
			cpu->ticks_running++;
			cpu->quantum_remaining--;

			if (cpu->burst_progress == cpu->t_cpu_burst && cpu->bursts_remaining == 1)
			{
				cpu->state = FINISHED;
				cpu->t_fin = tick;
				cpu->bursts_remaining = 0;
				finished_count++;
				cpu = NULL;
			}
			else if (tick >= cpu->t_deadline)
			{
				cpu->state = DEAD;
				cpu->t_fin = tick;
				finished_count++;
				cpu = NULL;
			}
			else if (cpu->burst_progress == cpu->t_cpu_burst)
			{
				cpu->bursts_remaining--;
				cpu->burst_progress = 0;
				cpu->io_remaining = cpu->io_wait;
				cpu->state = WAITING;
				queue_push(&queue, cpu);
				cpu = NULL;
			}
			else if (cpu->quantum_remaining == 0)
			{
				cpu->n_interruptions++;
				process_set_ready(cpu, tick);
				queue_push(&queue, cpu);
				cpu = NULL;
			}
		}

		if (cpu == NULL)
		{
			cpu = queue_pop_best_ready(&queue);
			if (cpu != NULL)
			{
				if (!cpu->has_been_dispatched)
				{
					cpu->first_dispatch_time = tick;
					cpu->has_been_dispatched = true;
				}
				cpu->state = RUNNING;
				cpu->quantum_remaining = q;
			}
		}


		tick++;
	}

	// Ordenar por PID para el output
	Process **por_pid = malloc(n_procesos * sizeof(Process *));
	for (int i = 0; i < n_procesos; i++)
	{
		por_pid[i] = &procesos[i];
	}
	for (int i = 1; i < n_procesos; i++)
	{
		Process *key = por_pid[i];
		int j = i - 1;
		while (j >= 0 && por_pid[j]->pid > key->pid)
		{
			por_pid[j + 1] = por_pid[j];
			j--;
		}
		por_pid[j + 1] = key;
	}

	FILE *output_file = fopen(argv[2], "w");

	for (int i = 0; i < n_procesos; i++)
	{
		Process *p = por_pid[i];
		unsigned int turnaround = p->t_fin - p->t_inicio;
		unsigned int waiting = turnaround - p->ticks_running;
		int response = p->has_been_dispatched
						   ? (int)(p->first_dispatch_time - p->t_inicio)
						   : -1;
		const char *state_str = (p->state == FINISHED) ? "FINISHED" : "DEAD";

		fprintf(output_file, "%s,%d,%s,%u,%u,%d,%u\n",
				p->name, p->pid, state_str,
				p->n_interruptions, turnaround, response, waiting);
	}

	fclose(output_file);
	free(por_pid);

	queue_free(&queue);
	for (int i = 0; i < n_procesos; i++)
	{
		process_destroy(&procesos[i]);
	}
	free(procesos);

	return 0;
}