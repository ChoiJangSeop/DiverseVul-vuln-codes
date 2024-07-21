void h2_mplx_task_done(h2_mplx *m, h2_task *task, h2_task **ptask)
{
    H2_MPLX_ENTER_ALWAYS(m);

    task_done(m, task);
    --m->tasks_active;
    
    if (m->join_wait) {
        apr_thread_cond_signal(m->join_wait);
    }
    if (ptask) {
        /* caller wants another task */
        *ptask = next_stream_task(m);
    }
    register_if_needed(m);

    H2_MPLX_LEAVE(m);
}