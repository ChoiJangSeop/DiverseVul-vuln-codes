grantpt (int fd)
{
  int retval = -1;
#ifdef PATH_MAX
  char _buf[PATH_MAX];
#else
  char _buf[512];
#endif
  char *buf = _buf;
  struct stat64 st;

  if (__builtin_expect (pts_name (fd, &buf, sizeof (_buf), &st), 0))
    {
      int save_errno = errno;

      /* Check, if the file descriptor is valid.  pts_name returns the
	 wrong errno number, so we cannot use that.  */
      if (__libc_fcntl (fd, F_GETFD) == -1 && errno == EBADF)
	return -1;

       /* If the filedescriptor is no TTY, grantpt has to set errno
	  to EINVAL.  */
       if (save_errno == ENOTTY)
	 __set_errno (EINVAL);
       else
	 __set_errno (save_errno);

       return -1;
    }

  /* Make sure that we own the device.  */
  uid_t uid = __getuid ();
  if (st.st_uid != uid)
    {
      if (__chown (buf, uid, st.st_gid) < 0)
	goto helper;
    }

  static int tty_gid = -1;
  if (__builtin_expect (tty_gid == -1, 0))
    {
      char *grtmpbuf;
      struct group grbuf;
      size_t grbuflen = __sysconf (_SC_GETGR_R_SIZE_MAX);
      struct group *p;

      /* Get the group ID of the special `tty' group.  */
      if (grbuflen == (size_t) -1L)
	/* `sysconf' does not support _SC_GETGR_R_SIZE_MAX.
	   Try a moderate value.  */
	grbuflen = 1024;
      grtmpbuf = (char *) __alloca (grbuflen);
      __getgrnam_r (TTY_GROUP, &grbuf, grtmpbuf, grbuflen, &p);
      if (p != NULL)
	tty_gid = p->gr_gid;
    }
  gid_t gid = tty_gid == -1 ? __getgid () : tty_gid;

  /* Make sure the group of the device is that special group.  */
  if (st.st_gid != gid)
    {
      if (__chown (buf, uid, gid) < 0)
	goto helper;
    }

  /* Make sure the permission mode is set to readable and writable by
     the owner, and writable by the group.  */
  if ((st.st_mode & ACCESSPERMS) != (S_IRUSR|S_IWUSR|S_IWGRP))
    {
      if (__chmod (buf, S_IRUSR|S_IWUSR|S_IWGRP) < 0)
	goto helper;
    }

  retval = 0;
  goto cleanup;

  /* We have to use the helper program.  */
 helper:;

  pid_t pid = __fork ();
  if (pid == -1)
    goto cleanup;
  else if (pid == 0)
    {
      /* Disable core dumps.  */
      struct rlimit rl = { 0, 0 };
      __setrlimit (RLIMIT_CORE, &rl);

      /* We pass the master pseudo terminal as file descriptor PTY_FILENO.  */
      if (fd != PTY_FILENO)
	if (__dup2 (fd, PTY_FILENO) < 0)
	  _exit (FAIL_EBADF);

#ifdef CLOSE_ALL_FDS
      CLOSE_ALL_FDS ();
#endif

      execle (_PATH_PT_CHOWN, basename (_PATH_PT_CHOWN), NULL, NULL);
      _exit (FAIL_EXEC);
    }
  else
    {
      int w;

      if (__waitpid (pid, &w, 0) == -1)
	goto cleanup;
      if (!WIFEXITED (w))
	__set_errno (ENOEXEC);
      else
	switch (WEXITSTATUS (w))
	  {
	  case 0:
	    retval = 0;
	    break;
	  case FAIL_EBADF:
	    __set_errno (EBADF);
	    break;
	  case FAIL_EINVAL:
	    __set_errno (EINVAL);
	    break;
	  case FAIL_EACCES:
	    __set_errno (EACCES);
	    break;
	  case FAIL_EXEC:
	    __set_errno (ENOEXEC);
	    break;
	  case FAIL_ENOMEM:
	    __set_errno (ENOMEM);
	    break;

	  default:
	    assert(! "getpt: internal error: invalid exit code from pt_chown");
	  }
    }

 cleanup:
  if (buf != _buf)
    free (buf);

  return retval;
}