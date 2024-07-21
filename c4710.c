ip_create_slave_core(interp, argc, argv)
    VALUE interp;
    int   argc;
    VALUE *argv;
{
    struct tcltkip *master = get_ip(interp);
    struct tcltkip *slave;
    /* struct tcltkip *slave = RbTk_ALLOC_N(struct tcltkip, 1); */
    VALUE safemode;
    VALUE name;
    VALUE new_ip;
    int safe;
    int thr_crit_bup;
    Tk_Window mainWin;

    /* ip is deleted? */
    if (deleted_ip(master)) {
        return rb_exc_new2(rb_eRuntimeError,
                           "deleted master cannot create a new slave");
    }

    name     = argv[0];
    safemode = argv[1];

    if (Tcl_IsSafe(master->ip) == 1) {
        safe = 1;
    } else if (safemode == Qfalse || NIL_P(safemode)) {
        safe = 0;
    } else {
        safe = 1;
    }

    thr_crit_bup = rb_thread_critical;
    rb_thread_critical = Qtrue;

#if 0
    /* init Tk */
    if (RTEST(with_tk)) {
        volatile VALUE exc;
        if (!tk_stubs_init_p()) {
            exc = tcltkip_init_tk(interp);
            if (!NIL_P(exc)) {
                rb_thread_critical = thr_crit_bup;
                return exc;
            }
        }
    }
#endif

    new_ip = TypedData_Make_Struct(CLASS_OF(interp), struct tcltkip,
				   &tcltkip_type, slave);
    /* create slave-ip */
#ifdef RUBY_USE_NATIVE_THREAD
    /* slave->tk_thread_id = 0; */
    slave->tk_thread_id = master->tk_thread_id; /* == current thread */
#endif
    slave->ref_count = 0;
    slave->allow_ruby_exit = 0;
    slave->return_value = 0;

    slave->ip = Tcl_CreateSlave(master->ip, StringValuePtr(name), safe);
    if (slave->ip == NULL) {
        rb_thread_critical = thr_crit_bup;
        return rb_exc_new2(rb_eRuntimeError,
                           "fail to create the new slave interpreter");
    }
#if TCL_MAJOR_VERSION >= 8
#if TCL_NAMESPACE_DEBUG
    slave->default_ns = Tcl_GetCurrentNamespace(slave->ip);
#endif
#endif
    rbtk_preserve_ip(slave);

    slave->has_orig_exit
        = Tcl_GetCommandInfo(slave->ip, "exit", &(slave->orig_exit_info));

    /* replace 'exit' command --> 'interp_exit' command */
    mainWin = (tk_stubs_init_p())? Tk_MainWindow(slave->ip): (Tk_Window)NULL;
#if TCL_MAJOR_VERSION >= 8
    DUMP1("Tcl_CreateObjCommand(\"exit\") --> \"interp_exit\"");
    Tcl_CreateObjCommand(slave->ip, "exit", ip_InterpExitObjCmd,
                         (ClientData)mainWin, (Tcl_CmdDeleteProc *)NULL);
#else /* TCL_MAJOR_VERSION < 8 */
    DUMP1("Tcl_CreateCommand(\"exit\") --> \"interp_exit\"");
    Tcl_CreateCommand(slave->ip, "exit", ip_InterpExitCommand,
                      (ClientData)mainWin, (Tcl_CmdDeleteProc *)NULL);
#endif

    /* replace vwait and tkwait */
    ip_replace_wait_commands(slave->ip, mainWin);

    /* wrap namespace command */
    ip_wrap_namespace_command(slave->ip);

    /* define command to replace cmds which depend on slave-slave's MainWin */
#if TCL_MAJOR_VERSION >= 8
    Tcl_CreateObjCommand(slave->ip, "__replace_slave_tk_commands__",
			 ip_rb_replaceSlaveTkCmdsObjCmd,
                         (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
#else /* TCL_MAJOR_VERSION < 8 */
    Tcl_CreateCommand(slave->ip, "__replace_slave_tk_commands__",
		      ip_rb_replaceSlaveTkCmdsCommand,
                      (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
#endif

    /* set finalizer */
    Tcl_CallWhenDeleted(slave->ip, ip_CallWhenDeleted, (ClientData)mainWin);

    rb_thread_critical = thr_crit_bup;

    return new_ip;
}