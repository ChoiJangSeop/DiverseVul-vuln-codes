read_stdin_handler(gpointer data, gint fd, GdkInputCondition condition)
{
    struct stdin_buf *input = (struct stdin_buf *)data;

    if (condition & GDK_INPUT_EXCEPTION) {
        g_print("input exception");
        input->count = 0;	/* EOF */
    }
    else if (condition & GDK_INPUT_READ) {
        /* read returns -1 for error, 0 for EOF and +ve for OK */
        input->count = read(fd, input->buf, input->len);
        if (input->count < 0)
            input->count = 0;
    }
    else {
        g_print("input condition unknown");
        input->count = 0;	/* EOF */
    }
}