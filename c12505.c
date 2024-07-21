int fuse_kern_mount(const char *mountpoint, struct fuse_args *args)
{
    struct mount_opts mo;
    int res = -1;
    char *mnt_opts = NULL;
#ifdef __SOLARIS__
    struct solaris_mount_opts smo;
    struct fuse_args sa = FUSE_ARGS_INIT(0, NULL);
#endif /* __SOLARIS__ */

    memset(&mo, 0, sizeof(mo));
#ifndef __SOLARIS__
    if (getuid())
	    mo.flags = MS_NOSUID | MS_NODEV;
#else /* __SOLARIS__ */
    mo.flags = 0;
    memset(&smo, 0, sizeof(smo));
    if (args != NULL) {
    	while (args->argv[sa.argc] != NULL)
		fuse_opt_add_arg(&sa, args->argv[sa.argc]);
    }
#endif /* __SOLARIS__ */

    if (args &&
        fuse_opt_parse(args, &mo, fuse_mount_opts, fuse_mount_opt_proc) == -1)
#ifndef __SOLARIS__
        return -1;
#else /* __SOLARIS__ */
        goto out; /* if SOLARIS, clean up 'sa' */

    /*
     * In Solaris, nosuid is equivalent to nosetuid + nodevices. We only
     * have MS_NOSUID for mount flags (no MS_(NO)SETUID, etc.). But if
     * we set that as a default, it restricts specifying just nosetuid
     * or nodevices; there is no way for the user to specify setuid +
     * nodevices or vice-verse. So we parse the existing options, then
     * add restrictive defaults if needed.
     */
    if (fuse_opt_parse(&sa, &smo, solaris_mnt_opts, NULL) == -1)
         goto out;
    if (smo.nosuid || (!smo.nodevices && !smo.devices
        && !smo.nosetuid && !smo.setuid)) {
        mo.flags |= MS_NOSUID;
    } else {
        /*
         * Defaults; if neither nodevices|devices,nosetuid|setuid has
         * been specified, add the default negative option string. If
         * both have been specified (i.e., -osuid,nosuid), leave them
         * alone; the last option will have precedence.
         */
        if (!smo.nodevices && !smo.devices)
             if (fuse_opt_add_opt(&mo.kernel_opts, "nodevices") == -1)
                 goto out;
        if (!smo.nosetuid && !smo.setuid)
            if (fuse_opt_add_opt(&mo.kernel_opts, "nosetuid") == -1)
                 goto out;
    }
#endif /* __SOLARIS__ */

    if (mo.allow_other && mo.allow_root) {
        fprintf(stderr, "fuse: 'allow_other' and 'allow_root' options are mutually exclusive\n");
        goto out;
    }
    res = 0;
    if (mo.ishelp)
        goto out;

    res = -1;
    if (get_mnt_flag_opts(&mnt_opts, mo.flags) == -1)
        goto out;
#ifndef __SOLARIS__
    if (!(mo.flags & MS_NODEV) && fuse_opt_add_opt(&mnt_opts, "dev") == -1)
        goto out;
    if (!(mo.flags & MS_NOSUID) && fuse_opt_add_opt(&mnt_opts, "suid") == -1)
        goto out;
    if (mo.kernel_opts && fuse_opt_add_opt(&mnt_opts, mo.kernel_opts) == -1)
        goto out;
    if (mo.mtab_opts &&  fuse_opt_add_opt(&mnt_opts, mo.mtab_opts) == -1)
        goto out;
    if (mo.fusermount_opts && fuse_opt_add_opt(&mnt_opts, mo.fusermount_opts) < 0)
        goto out;
    res = fusermount(0, 0, 0, mnt_opts ? mnt_opts : "", mountpoint);
#else /* __SOLARIS__ */
    if (mo.kernel_opts && fuse_opt_add_opt(&mnt_opts, mo.kernel_opts) == -1)
        goto out;
    if (mo.mtab_opts &&  fuse_opt_add_opt(&mnt_opts, mo.mtab_opts) == -1)
        goto out;
    res = fuse_mount_sys(mountpoint, &mo, mnt_opts);
    if (res == -2) {
        if (mo.fusermount_opts &&
            fuse_opt_add_opt(&mnt_opts, mo.fusermount_opts) == -1)
            goto out;

        if (mo.subtype) {
            char *tmp_opts = NULL;

            res = -1;
            if (fuse_opt_add_opt(&tmp_opts, mnt_opts) == -1 ||
                fuse_opt_add_opt(&tmp_opts, mo.subtype_opt) == -1) {
                free(tmp_opts);
                goto out;
            }

            res = fuse_mount_fusermount(mountpoint, tmp_opts, 1);
            free(tmp_opts);
            if (res == -1)
                res = fuse_mount_fusermount(mountpoint, mnt_opts, 0);
        } else {
            res = fuse_mount_fusermount(mountpoint, mnt_opts, 0);
        }
    }
#endif /* __SOLARIS__ */

out:
    free(mnt_opts);
#ifdef __SOLARIS__
    fuse_opt_free_args(&sa);
    free(mo.subtype);
    free(mo.subtype_opt);
#endif /* __SOLARIS__ */
    free(mo.fsname);
    free(mo.fusermount_opts);
    free(mo.kernel_opts);
    free(mo.mtab_opts);
    return res;
}