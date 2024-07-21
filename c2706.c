int main(int argc, char **argv)
{
    /*
     * We disallow passing of arguments which point to writable dirs
     * and other files possibly not accessible to calling user.
     * This way, the script will always use default values for these arguments.
     */
    char **pp = argv;
    char *arg;
    while ((arg = *++pp) != NULL)
    {
        /* Allow taking ids from stdin */
        if (strcmp(arg, "--ids=-") == 0)
            continue;

        if (strncmp(arg, "--exact", 7) == 0)
            continue;

        if (strncmp(arg, "--cache", 7) == 0)
            error_msg_and_die("bad option %s", arg);
        if (strncmp(arg, "--tmpdir", 8) == 0)
            error_msg_and_die("bad option %s", arg);
        if (strncmp(arg, "--ids", 5) == 0)
            error_msg_and_die("bad option %s", arg);
    }

    /* Switch real user/group to effective ones.
     * Otherwise yum library gets confused - gets EPERM (why??).
     */
    gid_t g = getegid();
    /* do setregid only if we have to, to not upset selinux needlessly */
    if (g != getgid())
        IGNORE_RESULT(setregid(g, g));
    uid_t u = geteuid();
    if (u != getuid())
    {
        IGNORE_RESULT(setreuid(u, u));
        /* We are suid'ed! */
        /* Prevent malicious user from messing up with suid'ed process: */
#if 1
// We forgot to sanitize PYTHONPATH. And who knows what else we forgot
// (especially considering *future* new variables of this kind).
// We switched to clearing entire environment instead:

        // However since we communicate through environment variables
        // we have to keep a whitelist of variables to keep.
        static const char *whitelist[] = {
            "REPORT_CLIENT_SLAVE", //  Check if the app is being run as a slave
            "LANG",
        };
        const size_t wlsize = sizeof(whitelist)/sizeof(char*);
        char *setlist[sizeof(whitelist)/sizeof(char*)] = { 0 };
        char *p = NULL;
        for (size_t i = 0; i < wlsize; i++)
            if ((p = getenv(whitelist[i])) != NULL)
                setlist[i] = xstrdup(p);

        // Now we can clear the environment
        clearenv();

        // And once again set whitelisted variables
        for (size_t i = 0; i < wlsize; i++)
            if (setlist[i] != NULL)
            {
                xsetenv(whitelist[i], setlist[i]);
                free(setlist[i]);
            }
#else
        /* Clear dangerous stuff from env */
        static const char forbid[] =
            "LD_LIBRARY_PATH" "\0"
            "LD_PRELOAD" "\0"
            "LD_TRACE_LOADED_OBJECTS" "\0"
            "LD_BIND_NOW" "\0"
            "LD_AOUT_LIBRARY_PATH" "\0"
            "LD_AOUT_PRELOAD" "\0"
            "LD_NOWARN" "\0"
            "LD_KEEPDIR" "\0"
        ;
        const char *p = forbid;
        do {
            unsetenv(p);
            p += strlen(p) + 1;
        } while (*p);
#endif
        /* Set safe PATH */
        // Adding configure --bindir and --sbindir to the PATH so that
        // abrt-action-install-debuginfo doesn't fail when spawning
        // abrt-action-trim-files
        char path_env[] = "PATH=/usr/sbin:/sbin:/usr/bin:/bin:"BIN_DIR":"SBIN_DIR;
        if (u != 0)
            strcpy(path_env, "PATH=/usr/bin:/bin:"BIN_DIR);
        putenv(path_env);
    }

    execvp(EXECUTABLE, argv);
    error_msg_and_die("Can't execute %s", EXECUTABLE);
}