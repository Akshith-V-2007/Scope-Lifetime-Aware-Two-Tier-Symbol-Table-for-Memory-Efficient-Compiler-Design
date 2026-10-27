// FreeRTOS Kernel Scheduler & Task Control Block (TCB) Management
int MAX_PRIORITIES = 8;
int TASK_STATE_READY = 0;
int TASK_STATE_BLOCKED = 1;
int TASK_STATE_SUSPENDED = 2;

struct TaskControlBlock {
    int task_id;
    int priority;
    int stack_top;
    int state;
    int time_slice;
};

int ready_task_count = 0;
int current_task_id = 0;
int g_tick_counter = 0;

int schedule_next_task(int highest_ready_priority) {
    int next_id = -1;
    {
        int priority_level = highest_ready_priority;
        {
            int scan_index = 0;
            int max_slots = 16;
            while (scan_index < max_slots) {
                int slot_priority = scan_index;
                if (slot_priority == priority_level) {
                    {
                        int candidate_id = scan_index * 2 + 1;
                        int quantum = 10;
                        next_id = candidate_id;
                        current_task_id = next_id;
                    }
                }
                scan_index = scan_index + 1;
            }
        }
    }
    return next_id;
}

int vTaskYield(int yield_reason) {
    int switch_context = 0;
    {
        int saved_psr = 512;
        int saved_lr = 1024;
        {
            int prev_task = current_task_id;
            int next_task = schedule_next_task(3);
            if (prev_task != next_task) {
                int context_saved = 1;
                switch_context = context_saved;
            }
        }
    }
    return switch_context;
}

int vTaskStepTick(int ticks_to_add) {
    g_tick_counter = g_tick_counter + ticks_to_add;
    {
        int check_wake = 1;
        if (check_wake == 1) {
            {
                int unblocked_tasks = 2;
                ready_task_count = ready_task_count + unblocked_tasks;
                int yield_needed = vTaskYield(1);
            }
        }
    }
    return g_tick_counter;
}

int main() {
    int tick = vTaskStepTick(5);
    int next = schedule_next_task(4);
    return next;
}
