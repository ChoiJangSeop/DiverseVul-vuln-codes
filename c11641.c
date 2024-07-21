int main(int argc, char **argv)
{
    int ret = -1, argc_utf8 = -1;
    char **argv_utf8 = NULL;

    init_commandline_arguments_utf8(&argc_utf8, &argv_utf8);
    ret = wvunpack_main(argc_utf8, argv_utf8);
    free_commandline_arguments_utf8(&argc_utf8, &argv_utf8);
    if (pause_mode) do_pause_mode ();
    return ret;
}