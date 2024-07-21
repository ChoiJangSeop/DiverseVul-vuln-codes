do_tabs(int *tab_list)
{
    int last = 1;
    int stop;

    putchar('\r');
    while ((stop = *tab_list++) > 0) {
	if (last < stop) {
	    while (last++ < stop) {
		if (last > max_cols)
		    break;
		putchar(' ');
	    }
	}
	if (stop <= max_cols) {
	    tputs(tparm(set_tab, stop), 1, putch);
	    last = stop;
	} else {
	    break;
	}
    }
    putchar('\r');
}