#include "scheduler.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "msg.h"

static const char *SCHED_NAMES[] = { "FIFO", "SJF", "RR", "MLFQ", NULL };
static sched_algo_en sched_algo = SCHED_FIFO;

int set_sched_algo(const char *name) {
    for (int i = 0; SCHED_NAMES[i] != NULL; i++) {
        if (strcasecmp(SCHED_NAMES[i], name) == 0) {
            sched_algo = (sched_algo_en) i;
            return sched_algo;
        }
    }
    return -1;
}

const char *get_sched_algo_str(void) {
    return SCHED_NAMES[sched_algo];
}

/**
 * Send DONE to the application and move the finished burst to the command queue.
 */
static void finish_burst(uint32_t current_time_ms, queue_t *cq, pcb_t *task) {
    msg_t msg = {
        .pid = task->pid,
        .request = PROCESS_REQUEST_DONE,
        .time_ms = current_time_ms
    };
    if (write(task->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
        perror("write");
    }
    enqueue_pcb(cq, task);
}

/**
 * FIFO (First-In, First-Out) — não preemptivo.
 *
 * O primeiro processo a chegar à ready queue corre até o seu burst de CPU
 * terminar; só então o CPU fica livre para o processo seguinte.
 *
 * TODO (alunos): acrescentar os algoritmos SJF, RR (time-slice = TIME_SLICE_MS)
 * e MLFQ, selecionados por `sched_algo` / `--sched`.
 */
int scheduler(uint32_t current_time_ms, queue_t *rq, queue_t *cq, pcb_t **cpu_task) {
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // 1. Verifica se o burst terminou
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            finish_burst(current_time_ms, cq, *cpu_task);
            *cpu_task = NULL;
        }
        // 2. Verifica preempção (Time-slice esgotado) para RR e MLFQ
        else if (sched_algo == SCHED_RR || sched_algo == SCHED_MLFQ) {
            uint32_t current_slice_used = current_time_ms - (*cpu_task)->slice_start_ms + TICKS_MS;

            if (current_slice_used >= TIME_SLICE_MS) {
                // Para o MLFQ, aqui penalizaríamos a prioridade da tarefa
                if (sched_algo == SCHED_MLFQ) {
                    (*cpu_task)->nice++; // Desce a prioridade (quanto maior o nice, menor a prioridade)
                }

                // Volta a colocar na ready queue (Preempção)
                enqueue_pcb(rq, *cpu_task);
                *cpu_task = NULL;
            }
        }
    }

    // 3. Se o CPU estiver livre, escolhe a próxima tarefa
    if (*cpu_task == NULL) {
        if (sched_algo == SCHED_FIFO || sched_algo == SCHED_RR) {
            *cpu_task = dequeue_pcb(rq); // RR usa FIFO puro para desencadeamento
        } else if (sched_algo == SCHED_SJF) {
            *cpu_task = dequeue_pcb_sjf(rq);
        } else if (sched_algo == SCHED_MLFQ) {
            *cpu_task = dequeue_pcb_mlfq(rq); // Precisarás criar esta função
        }

        if (*cpu_task) {
            (*cpu_task)->slice_start_ms = current_time_ms;

            printf("[Tempo: %d ms] CPU entregue ao PID %d (Nice: %d, Falta correr: %d ms)\n",
                   current_time_ms, (*cpu_task)->pid, (*cpu_task)->nice,
                   (*cpu_task)->time_ms - (*cpu_task)->ellapsed_time_ms);

            return 1;
        }
    }
    return 0;
}