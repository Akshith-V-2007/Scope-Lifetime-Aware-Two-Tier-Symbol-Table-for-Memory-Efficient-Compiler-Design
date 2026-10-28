// Zephyr RTOS Microkernel Thread Dispatcher & Synopsys ARC Scheduler Core
// Source: Zephyr Project subsys/kernel/sched.c (adapted for Mini-C parser)

struct k_thread {
    int base_prio;
    int current_prio;
    int thread_state;
    int cpu_affinity;
    int slice_ticks_remaining;
    int stack_pointer;
    int thread_id;
    int swap_required;
};

struct SchedReadyQueue {
    int highest_prio;
    int active_thread_count;
    int bitmask_prio_levels;
    int lock_count;
};

int _THREAD_STATE_DUMMY = 0;
int _THREAD_STATE_READY = 1;
int _THREAD_STATE_SUSPENDED = 2;
int _THREAD_STATE_PREEMPTED = 4;
int _THREAD_STATE_DEAD = 8;

int G_KERNEL_TICKS = 0;
int G_CURRENT_THREAD_ID = 0;
int G_CONTEXT_SWITCHES = 0;
int G_MAX_PRIORITIES = 32;

int z_sched_prio_find_highest(int bitmask) {
    int prio = 0;
    while (prio < 32) {
        int mask = 1;
        {
            int s = 0;
            for (s = 0; s < prio; s = s + 1) {
                mask = mask * 2;
            }
            if ((bitmask & mask) != 0) {
                return prio;
            }
        }
        prio = prio + 1;
    }
    return 31;
}

int z_sched_thread_ready(struct k_thread* thread, struct SchedReadyQueue* rq) {
    int prio = thread->current_prio;
    int state = thread->thread_state;

    if (state != _THREAD_STATE_READY) {
        {
            thread->thread_state = _THREAD_STATE_READY;
            rq->active_thread_count = rq->active_thread_count + 1;

            int mask = 1;
            {
                int k = 0;
                for (k = 0; k < prio; k = k + 1) {
                    mask = mask * 2;
                }
                rq->bitmask_prio_levels = rq->bitmask_prio_levels + mask;
            }

            if (prio < rq->highest_prio) {
                rq->highest_prio = prio;
            }
        }
    }
    return 0;
}

int z_sched_time_slice(struct k_thread* current, struct SchedReadyQueue* rq, int elapsed_ticks) {
    int should_preempt = 0;
    int current_ticks = current->slice_ticks_remaining;

    if (current_ticks > elapsed_ticks) {
        current->slice_ticks_remaining = current_ticks - elapsed_ticks;
    } else {
        {
            int slice_quantum = 10;
            current->slice_ticks_remaining = slice_quantum;

            int best_prio = z_sched_prio_find_highest(rq->bitmask_prio_levels);
            if (best_prio <= current->current_prio) {
                {
                    int swap_flag = 1;
                    current->swap_required = swap_flag;
                    should_preempt = 1;
                    G_CONTEXT_SWITCHES = G_CONTEXT_SWITCHES + 1;
                }
            }
        }
    }
    return should_preempt;
}

int z_sched_dispatch(struct k_thread* curr_th, struct k_thread* next_th, struct SchedReadyQueue* rq) {
    int switched = 0;
    int lock_level = rq->lock_count;

    if (lock_level == 0) {
        {
            int curr_id = curr_th->thread_id;
            int next_id = next_th->thread_id;

            if (curr_id != next_id) {
                {
                    int sp_save = curr_th->stack_pointer;
                    int sp_restore = next_th->stack_pointer;

                    G_CURRENT_THREAD_ID = next_id;
                    switched = 1;
                }
            }
        }
    }
    return switched;
}

int z_sched_run_pipeline(int thread_count, int simulation_cycles) {
    struct SchedReadyQueue rq;
    rq.highest_prio = 31;
    rq.active_thread_count = 0;
    rq.bitmask_prio_levels = 0;
    rq.lock_count = 0;

    struct k_thread pool[16];
    int t = 0;
    for (t = 0; t < thread_count; t = t + 1) {
        {
            pool[t].thread_id = t + 1;
            pool[t].base_prio = t % 8;
            pool[t].current_prio = pool[t].base_prio;
            pool[t].thread_state = _THREAD_STATE_SUSPENDED;
            pool[t].cpu_affinity = 0;
            pool[t].slice_ticks_remaining = 10;
            pool[t].stack_pointer = 65536 + t * 4096;
            pool[t].swap_required = 0;

            z_sched_thread_ready(&pool[t], &rq);
        }
    }

    int cycle = 0;
    int current_index = 0;
    for (cycle = 0; cycle < simulation_cycles; cycle = cycle + 1) {
        {
            int elapsed = 2;
            G_KERNEL_TICKS = G_KERNEL_TICKS + elapsed;

            int preempt = z_sched_time_slice(&pool[current_index], &rq, elapsed);
            if (preempt != 0) {
                {
                    int next_index = (current_index + 1) % thread_count;
                    z_sched_dispatch(&pool[current_index], &pool[next_index], &rq);
                    current_index = next_index;
                }
            }
        }
    }

    return G_CONTEXT_SWITCHES;
}
