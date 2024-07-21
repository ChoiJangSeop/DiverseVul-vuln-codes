ip_init(argc, argv, self)
    int   argc;
    VALUE *argv;
    VALUE self;
{
    struct tcltkip *ptr;        /* tcltkip data struct */
    VALUE argv0, opts;
    int cnt;
    int st;
    int with_tk = 1;
    Tk_Window mainWin = (Tk_Window)NULL;

    /* create object */
    TypedData_Get_Struct(self, struct tcltkip, &tcltkip_type, ptr);
    if (DATA_PTR(self)) {
	rb_raise(rb_eArgError, "already initialized interpreter");
    }
    ptr = ALLOC(struct tcltkip);
    /* ptr = RbTk_ALLOC_N(struct tcltkip, 1); */
    DATA_PTR(self) = ptr;
#ifdef RUBY_USE_NATIVE_THREAD
    ptr->tk_thread_id = 0;
#endif
    ptr->ref_count = 0;
    ptr->allow_ruby_exit = 1;
    ptr->return_value = 0;

    /* from Tk_Main() */
    DUMP1("Tcl_CreateInterp");
    ptr->ip = ruby_tcl_create_ip_and_stubs_init(&st);
    if (ptr->ip == NULL) {
        switch(st) {
        case TCLTK_STUBS_OK:
            break;
        case NO_TCL_DLL:
            rb_raise(rb_eLoadError, "tcltklib: fail to open tcl_dll");
        case NO_FindExecutable:
            rb_raise(rb_eLoadError, "tcltklib: can't find Tcl_FindExecutable");
        case NO_CreateInterp:
            rb_raise(rb_eLoadError, "tcltklib: can't find Tcl_CreateInterp()");
        case NO_DeleteInterp:
            rb_raise(rb_eLoadError, "tcltklib: can't find Tcl_DeleteInterp()");
        case FAIL_CreateInterp:
            rb_raise(rb_eRuntimeError, "tcltklib: fail to create a new IP");
        case FAIL_Tcl_InitStubs:
            rb_raise(rb_eRuntimeError, "tcltklib: fail to Tcl_InitStubs()");
        default:
            rb_raise(rb_eRuntimeError, "tcltklib: unknown error(%d) on ruby_tcl_create_ip_and_stubs_init", st);
        }
    }

#if TCL_MAJOR_VERSION >= 8
#if TCL_NAMESPACE_DEBUG
    DUMP1("get current namespace");
    if ((ptr->default_ns = Tcl_GetCurrentNamespace(ptr->ip))
        == (Tcl_Namespace*)NULL) {
      rb_raise(rb_eRuntimeError, "a new Tk interpreter has a NULL namespace");
    }
#endif
#endif

    rbtk_preserve_ip(ptr);
    DUMP2("IP ref_count = %d", ptr->ref_count);
    current_interp = ptr->ip;

    ptr->has_orig_exit
        = Tcl_GetCommandInfo(ptr->ip, "exit", &(ptr->orig_exit_info));

#if defined CREATE_RUBYTK_KIT || defined CREATE_RUBYKIT
    call_tclkit_init_script(current_interp);

# if 10 * TCL_MAJOR_VERSION + TCL_MINOR_VERSION > 84
    {
      Tcl_DString encodingName;
      Tcl_GetEncodingNameFromEnvironment(&encodingName);
      if (strcmp(Tcl_DStringValue(&encodingName), Tcl_GetEncodingName(NULL))) {
	/* fails, so we set a variable and do it in the boot.tcl script */
	Tcl_SetSystemEncoding(NULL, Tcl_DStringValue(&encodingName));
      }
      Tcl_SetVar(current_interp, "tclkit_system_encoding", Tcl_DStringValue(&encodingName), 0);
      Tcl_DStringFree(&encodingName);
    }
# endif
#endif

    /* set variables */
    Tcl_Eval(ptr->ip, "set argc 0; set argv {}; set argv0 tcltklib.so");

    cnt = rb_scan_args(argc, argv, "02", &argv0, &opts);
    switch(cnt) {
    case 2:
        /* options */
        if (NIL_P(opts) || opts == Qfalse) {
            /* without Tk */
            with_tk = 0;
        } else {
            /* Tcl_SetVar(ptr->ip, "argv", StringValuePtr(opts), 0); */
            Tcl_SetVar(ptr->ip, "argv", StringValuePtr(opts), TCL_GLOBAL_ONLY);
	    Tcl_Eval(ptr->ip, "set argc [llength $argv]");
        }
    case 1:
        /* argv0 */
        if (!NIL_P(argv0)) {
            if (strncmp(StringValuePtr(argv0), "-e", 3) == 0
                || strncmp(StringValuePtr(argv0), "-", 2) == 0) {
                Tcl_SetVar(ptr->ip, "argv0", "ruby", TCL_GLOBAL_ONLY);
            } else {
                /* Tcl_SetVar(ptr->ip, "argv0", StringValuePtr(argv0), 0); */
                Tcl_SetVar(ptr->ip, "argv0", StringValuePtr(argv0),
                           TCL_GLOBAL_ONLY);
            }
        }
    case 0:
        /* no args */
        ;
    }

    /* from Tcl_AppInit() */
    DUMP1("Tcl_Init");
#if (defined CREATE_RUBYTK_KIT || defined CREATE_RUBYKIT) && (!defined KIT_LITE) && (10 * TCL_MAJOR_VERSION + TCL_MINOR_VERSION == 85)
    /*************************************************************************/
    /*  FIX ME (2010/06/28)                                                  */
    /*    Don't use ::chan command for Mk4tcl + tclvfs-1.4 on Tcl8.5.        */
    /*    It fails to access VFS files because of vfs::zstream.              */
    /*    So, force to use ::rechan by temporarily hiding ::chan.            */
    /*************************************************************************/
    Tcl_Eval(ptr->ip, "catch {rename ::chan ::_tmp_chan}");
    if (Tcl_Init(ptr->ip) == TCL_ERROR) {
        rb_raise(rb_eRuntimeError, "%s", Tcl_GetStringResult(ptr->ip));
    }
    Tcl_Eval(ptr->ip, "catch {rename ::_tmp_chan ::chan}");
#else
    if (Tcl_Init(ptr->ip) == TCL_ERROR) {
        rb_raise(rb_eRuntimeError, "%s", Tcl_GetStringResult(ptr->ip));
    }
#endif

    st = ruby_tcl_stubs_init();
    /* from Tcl_AppInit() */
    if (with_tk) {
        DUMP1("Tk_Init");
        st = ruby_tk_stubs_init(ptr->ip);
        switch(st) {
        case TCLTK_STUBS_OK:
            break;
        case NO_Tk_Init:
            rb_raise(rb_eLoadError, "tcltklib: can't find Tk_Init()");
        case FAIL_Tk_Init:
            rb_raise(rb_eRuntimeError, "tcltklib: fail to Tk_Init(). %s",
                     Tcl_GetStringResult(ptr->ip));
        case FAIL_Tk_InitStubs:
            rb_raise(rb_eRuntimeError, "tcltklib: fail to Tk_InitStubs(). %s",
                     Tcl_GetStringResult(ptr->ip));
        default:
            rb_raise(rb_eRuntimeError, "tcltklib: unknown error(%d) on ruby_tk_stubs_init", st);
        }

        DUMP1("Tcl_StaticPackage(\"Tk\")");
#if TCL_MAJOR_VERSION >= 8
        Tcl_StaticPackage(ptr->ip, "Tk", Tk_Init, Tk_SafeInit);
#else /* TCL_MAJOR_VERSION < 8 */
        Tcl_StaticPackage(ptr->ip, "Tk", Tk_Init,
                          (Tcl_PackageInitProc *) NULL);
#endif

#ifdef RUBY_USE_NATIVE_THREAD
        /* set Tk thread ID */
        ptr->tk_thread_id = Tcl_GetCurrentThread();
#endif
        /* get main window */
        mainWin = Tk_MainWindow(ptr->ip);
        Tk_Preserve((ClientData)mainWin);
    }

    /* add ruby command to the interpreter */
#if TCL_MAJOR_VERSION >= 8
    DUMP1("Tcl_CreateObjCommand(\"ruby\")");
    Tcl_CreateObjCommand(ptr->ip, "ruby", ip_ruby_eval, (ClientData)NULL,
                         (Tcl_CmdDeleteProc *)NULL);
    DUMP1("Tcl_CreateObjCommand(\"ruby_eval\")");
    Tcl_CreateObjCommand(ptr->ip, "ruby_eval", ip_ruby_eval, (ClientData)NULL,
                         (Tcl_CmdDeleteProc *)NULL);
    DUMP1("Tcl_CreateObjCommand(\"ruby_cmd\")");
    Tcl_CreateObjCommand(ptr->ip, "ruby_cmd", ip_ruby_cmd, (ClientData)NULL,
                         (Tcl_CmdDeleteProc *)NULL);
#else /* TCL_MAJOR_VERSION < 8 */
    DUMP1("Tcl_CreateCommand(\"ruby\")");
    Tcl_CreateCommand(ptr->ip, "ruby", ip_ruby_eval, (ClientData)NULL,
                      (Tcl_CmdDeleteProc *)NULL);
    DUMP1("Tcl_CreateCommand(\"ruby_eval\")");
    Tcl_CreateCommand(ptr->ip, "ruby_eval", ip_ruby_eval, (ClientData)NULL,
                      (Tcl_CmdDeleteProc *)NULL);
    DUMP1("Tcl_CreateCommand(\"ruby_cmd\")");
    Tcl_CreateCommand(ptr->ip, "ruby_cmd", ip_ruby_cmd, (ClientData)NULL,
                      (Tcl_CmdDeleteProc *)NULL);
#endif

    /* add 'interp_exit', 'ruby_exit' and replace 'exit' command */
#if TCL_MAJOR_VERSION >= 8
    DUMP1("Tcl_CreateObjCommand(\"interp_exit\")");
    Tcl_CreateObjCommand(ptr->ip, "interp_exit", ip_InterpExitObjCmd,
                         (ClientData)mainWin, (Tcl_CmdDeleteProc *)NULL);
    DUMP1("Tcl_CreateObjCommand(\"ruby_exit\")");
    Tcl_CreateObjCommand(ptr->ip, "ruby_exit", ip_RubyExitObjCmd,
                         (ClientData)mainWin, (Tcl_CmdDeleteProc *)NULL);
    DUMP1("Tcl_CreateObjCommand(\"exit\") --> \"ruby_exit\"");
    Tcl_CreateObjCommand(ptr->ip, "exit", ip_RubyExitObjCmd,
                         (ClientData)mainWin, (Tcl_CmdDeleteProc *)NULL);
#else /* TCL_MAJOR_VERSION < 8 */
    DUMP1("Tcl_CreateCommand(\"interp_exit\")");
    Tcl_CreateCommand(ptr->ip, "interp_exit", ip_InterpExitCommand,
                      (ClientData)mainWin, (Tcl_CmdDeleteProc *)NULL);
    DUMP1("Tcl_CreateCommand(\"ruby_exit\")");
    Tcl_CreateCommand(ptr->ip, "ruby_exit", ip_RubyExitCommand,
                      (ClientData)mainWin, (Tcl_CmdDeleteProc *)NULL);
    DUMP1("Tcl_CreateCommand(\"exit\") --> \"ruby_exit\"");
    Tcl_CreateCommand(ptr->ip, "exit", ip_RubyExitCommand,
                      (ClientData)mainWin, (Tcl_CmdDeleteProc *)NULL);
#endif

    /* replace vwait and tkwait */
    ip_replace_wait_commands(ptr->ip, mainWin);

    /* wrap namespace command */
    ip_wrap_namespace_command(ptr->ip);

    /* define command to replace commands which depend on slave's MainWindow */
#if TCL_MAJOR_VERSION >= 8
    Tcl_CreateObjCommand(ptr->ip, "__replace_slave_tk_commands__",
			 ip_rb_replaceSlaveTkCmdsObjCmd,
                         (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
#else /* TCL_MAJOR_VERSION < 8 */
    Tcl_CreateCommand(ptr->ip, "__replace_slave_tk_commands__",
		      ip_rb_replaceSlaveTkCmdsCommand,
                      (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
#endif

    /* set finalizer */
    Tcl_CallWhenDeleted(ptr->ip, ip_CallWhenDeleted, (ClientData)mainWin);

    if (mainWin != (Tk_Window)NULL) {
        Tk_Release((ClientData)mainWin);
    }

    return self;
}