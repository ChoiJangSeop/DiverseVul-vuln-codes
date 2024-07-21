int main(int argc, char *argv[])
{
    int exit_status;
    int code = 1, code1;
    void *instance;
    int nargc;
    char **nargv;
    char dformat[64];
    int exit_code;
    gboolean use_gui;

    /* Gtk initialisation */
    gtk_set_locale();
    use_gui = gtk_init_check(&argc, &argv);

    /* insert display device parameters as first arguments */
    sprintf(dformat, "-dDisplayFormat=%d",
            DISPLAY_COLORS_RGB | DISPLAY_ALPHA_NONE | DISPLAY_DEPTH_8 |
            DISPLAY_BIGENDIAN | DISPLAY_TOPFIRST);
    nargc = argc + 1;
    nargv = (char **)malloc((nargc + 1) * sizeof(char *));
    nargv[0] = argv[0];
    nargv[1] = dformat;
    memcpy(&nargv[2], &argv[1], argc * sizeof(char *));

    /* run Ghostscript */
    if ((code = gsapi_new_instance(&instance, NULL)) == 0) {
        gsapi_set_stdio(instance, gsdll_stdin, gsdll_stdout, gsdll_stderr);
        if (use_gui)
            gsapi_set_display_callback(instance, &display);
        code = gsapi_init_with_args(instance, nargc, nargv);

        if (code == 0)
            code = gsapi_run_string(instance, start_string, 0, &exit_code);
        code1 = gsapi_exit(instance);
        if (code == 0 || code == e_Quit)
            code = code1;
        if (code == e_Quit)
            code = 0;	/* user executed 'quit' */

        gsapi_delete_instance(instance);
    }

    exit_status = 0;
    switch (code) {
        case 0:
        case e_Info:
        case e_Quit:
            break;
        case e_Fatal:
            exit_status = 1;
            break;
        default:
            exit_status = 255;
    }

    return exit_status;
}