int h2_mplx_awaits_data(h2_mplx *m)
{
    int waiting = 1;
     
    H2_MPLX_ENTER_ALWAYS(m);

    if (h2_ihash_empty(m->streams)) {
        waiting = 0;
    }
    else if (!m->tasks_active && !h2_ififo_count(m->readyq)
             && h2_iq_empty(m->q)) {
        waiting = 0;
    }

    H2_MPLX_LEAVE(m);
    return waiting;
}