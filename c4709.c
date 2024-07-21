alloc_invoke_arguments(argc, argv)
    int argc;
    VALUE *argv;
{
    int i;
    int thr_crit_bup;

#if TCL_MAJOR_VERSION >= 8
    Tcl_Obj **av;
#else /* TCL_MAJOR_VERSION < 8 */
    char **av;
#endif

    thr_crit_bup = rb_thread_critical;
    rb_thread_critical = Qtrue;

    /* memory allocation */
#if TCL_MAJOR_VERSION >= 8
    /* av = ALLOC_N(Tcl_Obj *, argc+1);*/ /* XXXXXXXXXX */
    av = RbTk_ALLOC_N(Tcl_Obj *, (argc+1));
#if 0 /* use Tcl_Preserve/Release */
    Tcl_Preserve((ClientData)av); /* XXXXXXXX */
#endif
    for (i = 0; i < argc; ++i) {
        av[i] = get_obj_from_str(argv[i]);
        Tcl_IncrRefCount(av[i]);
    }
    av[argc] = NULL;

#else /* TCL_MAJOR_VERSION < 8 */
    /* string interface */
    /* av = ALLOC_N(char *, argc+1); */
    av = RbTk_ALLOC_N(char *, (argc+1));
#if 0 /* use Tcl_Preserve/Release */
    Tcl_Preserve((ClientData)av); /* XXXXXXXX */
#endif
    for (i = 0; i < argc; ++i) {
        av[i] = strdup(StringValuePtr(argv[i]));
    }
    av[argc] = NULL;
#endif

    rb_thread_critical = thr_crit_bup;

    return av;
}