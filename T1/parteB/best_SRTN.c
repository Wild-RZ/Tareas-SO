#include <stdio.h>
#include <stdlib.h>
#include "SRTN.h"


void srtf(Process processes[], int n, int *total_interruptions, int threshold) {
    int current_time = 0, completed = 0;
    int last_idx = -1; 
    *total_interruptions = 0;

    while (completed < n) {
        int idx = -1;

        // busca el proceso disponible con el menor tiempo restante + threshold
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time <= current_time && processes[i].remaining_time > 0) {
                if (idx == -1 || processes[i].remaining_time + threshold < processes[idx].remaining_time) {
                    idx = i;
                }
            }
        }

        if (idx != -1) {
            if (last_idx != -1 && last_idx != idx && processes[last_idx].remaining_time > 0) {
                current_time = current_time + CONTEXT_SWITCH_OVERHEAD;
                processes[last_idx].interruptions++;
                (*total_interruptions)++;
            }

            processes[idx].remaining_time--;
            current_time++;
            last_idx = idx; 

            
            if (processes[idx].remaining_time == 0) {
                processes[idx].completion_time = current_time;
                processes[idx].turnaround_time = current_time - processes[idx].arrival_time;
                processes[idx].waiting_time = processes[idx].turnaround_time - processes[idx].burst_time;
                completed++;
            }
        } else {
            current_time++;
            last_idx = -1;
        }
    }
}

void print_results(Process processes[], int n, int total_interruptions) {
    float total_wt = 0, total_tat = 0;

    for (int i = 0; i < n; i++) {
        total_wt += processes[i].waiting_time;
        total_tat += processes[i].turnaround_time;
        printf("P%d CT: %d WT: %d TAT: %d Interruptions: %d\n", 
               processes[i].id, 
               processes[i].completion_time, 
               processes[i].waiting_time, 
               processes[i].turnaround_time,
               processes[i].interruptions);
    }

    printf("Numero de interrupciones: %d  \nTurnaround promedio: %.2f  \nWaitingtime promedio: %.2f  \n", 
           total_interruptions , total_tat / n, total_wt / n);
}

int main() {
    int n;
    printf("Enter number of processes: ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        return 1;
    }

    Process *processes = (Process *)malloc(n * sizeof(Process));

    for (int i = 0; i < n; i++) {
        processes[i].id = i + 1;
        printf("Enter arrival and burst time for P%d: ", i + 1);
        scanf("%d %d", &processes[i].arrival_time, &processes[i].burst_time);
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].interruptions = 0;
    }

    int threshold;
    printf("Enter number of threshold: ");
    scanf("%d", &threshold);
    int total_interruptions = 0;
    srtf(processes, n, &total_interruptions, threshold);
    print_results(processes, n, total_interruptions);

    free(processes);
    return 0;
}