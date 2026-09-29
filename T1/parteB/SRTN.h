#define CONTEXT_SWITCH_OVERHEAD 2

typedef struct {
    int id;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int interruptions;
} Process;
