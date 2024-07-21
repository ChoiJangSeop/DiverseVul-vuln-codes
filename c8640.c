choose_windows(s)
const char *s;
{
    register int i;

    for (i = 0; winchoices[i].procs; i++) {
        if ('+' == winchoices[i].procs->name[0])
            continue;
        if ('-' == winchoices[i].procs->name[0])
            continue;
        if (!strcmpi(s, winchoices[i].procs->name)) {
            windowprocs = *winchoices[i].procs;

            if (last_winchoice && last_winchoice->ini_routine)
                (*last_winchoice->ini_routine)(WININIT_UNDO);
            if (winchoices[i].ini_routine)
                (*winchoices[i].ini_routine)(WININIT);
            last_winchoice = &winchoices[i];
            return;
        }
    }

    if (!windowprocs.win_raw_print)
        windowprocs.win_raw_print = def_raw_print;
    if (!windowprocs.win_wait_synch)
        /* early config file error processing routines call this */
        windowprocs.win_wait_synch = def_wait_synch;

    if (!winchoices[0].procs) {
        raw_printf("No window types?");
        nh_terminate(EXIT_FAILURE);
    }
    if (!winchoices[1].procs) {
        config_error_add(
                     "Window type %s not recognized.  The only choice is: %s",
                         s, winchoices[0].procs->name);
    } else {
        char buf[BUFSZ];
        boolean first = TRUE;

        buf[0] = '\0';
        for (i = 0; winchoices[i].procs; i++) {
            if ('+' == winchoices[i].procs->name[0])
                continue;
            if ('-' == winchoices[i].procs->name[0])
                continue;
            Sprintf(eos(buf), "%s%s",
                    first ? "" : ", ", winchoices[i].procs->name);
            first = FALSE;
        }
        config_error_add("Window type %s not recognized.  Choices are:  %s",
                         s, buf);
    }

    if (windowprocs.win_raw_print == def_raw_print
            || WINDOWPORT("safe-startup"))
        nh_terminate(EXIT_SUCCESS);
}