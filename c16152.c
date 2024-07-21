gsdll_stdin(void *instance, char *buf, int len)
{
    struct stdin_buf input;
    gint input_tag;

    input.len = len;
    input.buf = buf;
    input.count = -1;

    input_tag = gdk_input_add(fileno(stdin),
        (GdkInputCondition)(GDK_INPUT_READ | GDK_INPUT_EXCEPTION),
        read_stdin_handler, &input);
    while (input.count < 0)
        gtk_main_iteration_do(TRUE);
    gdk_input_remove(input_tag);

    return input.count;
}