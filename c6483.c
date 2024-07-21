cupsdCreateProfile(int job_id,		/* I - Job ID or 0 for none */
                   int allow_networking)/* I - Allow networking off machine? */
{
#ifdef HAVE_SANDBOX_H
  cups_file_t		*fp;		/* File pointer */
  char			profile[1024],	/* File containing the profile */
			bin[1024],	/* Quoted ServerBin */
			cache[1024],	/* Quoted CacheDir */
			domain[1024],	/* Domain socket, if any */
			request[1024],	/* Quoted RequestRoot */
			root[1024],	/* Quoted ServerRoot */
			state[1024],	/* Quoted StateDir */
			temp[1024];	/* Quoted TempDir */
  const char		*nodebug;	/* " (with no-log)" for no debug */
  cupsd_listener_t	*lis;		/* Current listening socket */


  if (!UseSandboxing || Sandboxing == CUPSD_SANDBOXING_OFF)
  {
   /*
    * Only use sandbox profiles as root...
    */

    cupsdLogMessage(CUPSD_LOG_DEBUG2, "cupsdCreateProfile(job_id=%d, allow_networking=%d) = NULL", job_id, allow_networking);

    return (NULL);
  }

  if ((fp = cupsTempFile2(profile, sizeof(profile))) == NULL)
  {
    cupsdLogMessage(CUPSD_LOG_DEBUG2, "cupsdCreateProfile(job_id=%d, allow_networking=%d) = NULL", job_id, allow_networking);
    cupsdLogMessage(CUPSD_LOG_ERROR, "Unable to create security profile: %s",
                    strerror(errno));
    return (NULL);
  }

  fchown(cupsFileNumber(fp), RunUser, Group);
  fchmod(cupsFileNumber(fp), 0640);

  cupsd_requote(bin, ServerBin, sizeof(bin));
  cupsd_requote(cache, CacheDir, sizeof(cache));
  cupsd_requote(request, RequestRoot, sizeof(request));
  cupsd_requote(root, ServerRoot, sizeof(root));
  cupsd_requote(state, StateDir, sizeof(state));
  cupsd_requote(temp, TempDir, sizeof(temp));

  nodebug = LogLevel < CUPSD_LOG_DEBUG ? " (with no-log)" : "";

  cupsFilePuts(fp, "(version 1)\n");
  if (Sandboxing == CUPSD_SANDBOXING_STRICT)
    cupsFilePuts(fp, "(deny default)\n");
  else
    cupsFilePuts(fp, "(allow default)\n");
  if (LogLevel >= CUPSD_LOG_DEBUG)
    cupsFilePuts(fp, "(debug deny)\n");
  cupsFilePuts(fp, "(import \"system.sb\")\n");
  cupsFilePuts(fp, "(import \"com.apple.corefoundation.sb\")\n");
  cupsFilePuts(fp, "(system-network)\n");
  cupsFilePuts(fp, "(allow mach-per-user-lookup)\n");
  cupsFilePuts(fp, "(allow ipc-posix-sem)\n");
  cupsFilePuts(fp, "(allow ipc-posix-shm)\n");
  cupsFilePuts(fp, "(allow ipc-sysv-shm)\n");
  cupsFilePuts(fp, "(allow mach-lookup)\n");
  if (!RunUser)
    cupsFilePrintf(fp,
		   "(deny file-write* file-read-data file-read-metadata\n"
		   "  (regex"
		   " #\"^/Users$\""
		   " #\"^/Users/\""
		   ")%s)\n", nodebug);
  cupsFilePrintf(fp,
                 "(deny file-write*\n"
                 "  (regex"
		 " #\"^%s$\""		/* ServerRoot */
		 " #\"^%s/\""		/* ServerRoot/... */
		 " #\"^/private/etc$\""
		 " #\"^/private/etc/\""
		 " #\"^/usr/local/etc$\""
		 " #\"^/usr/local/etc/\""
		 " #\"^/Library$\""
		 " #\"^/Library/\""
		 " #\"^/System$\""
		 " #\"^/System/\""
		 ")%s)\n",
		 root, root, nodebug);
  /* Specifically allow applications to stat RequestRoot and some other system folders */
  cupsFilePrintf(fp,
                 "(allow file-read-metadata\n"
                 "  (regex"
		 " #\"^/$\""		/* / */
		 " #\"^/usr$\""		/* /usr */
		 " #\"^/Library$\""	/* /Library */
		 " #\"^/Library/Printers$\""	/* /Library/Printers */
		 " #\"^%s$\""		/* RequestRoot */
		 "))\n",
		 request);
  /* Read and write TempDir, CacheDir, and other common folders */
  cupsFilePuts(fp,
	       "(allow file-write* file-read-data file-read-metadata\n"
	       "  (regex"
	       " #\"^/private/var/db/\""
	       " #\"^/private/var/folders/\""
	       " #\"^/private/var/lib/\""
	       " #\"^/private/var/log/\""
	       " #\"^/private/var/mysql/\""
	       " #\"^/private/var/run/\""
	       " #\"^/private/var/spool/\""
	       " #\"^/Library/Application Support/\""
	       " #\"^/Library/Caches/\""
	       " #\"^/Library/Logs/\""
	       " #\"^/Library/Preferences/\""
	       " #\"^/Library/WebServer/\""
	       " #\"^/Users/Shared/\""
	       "))\n");
  cupsFilePrintf(fp,
		 "(deny file-write*\n"
		 "       (regex #\"^%s$\")%s)\n",
		 request, nodebug);
  cupsFilePrintf(fp,
		 "(deny file-write* file-read-data file-read-metadata\n"
		 "       (regex #\"^%s/\")%s)\n",
		 request, nodebug);
  cupsFilePrintf(fp,
                 "(allow file-write* file-read-data file-read-metadata\n"
                 "  (regex"
		 " #\"^%s$\""		/* TempDir */
		 " #\"^%s/\""		/* TempDir/... */
		 " #\"^%s$\""		/* CacheDir */
		 " #\"^%s/\""		/* CacheDir/... */
		 " #\"^%s$\""		/* StateDir */
		 " #\"^%s/\""		/* StateDir/... */
		 "))\n",
		 temp, temp, cache, cache, state, state);
  /* Read common folders */
  cupsFilePrintf(fp,
                 "(allow file-read-data file-read-metadata\n"
                 "  (regex"
                 " #\"^/AppleInternal$\""
                 " #\"^/AppleInternal/\""
                 " #\"^/bin$\""		/* /bin */
                 " #\"^/bin/\""		/* /bin/... */
                 " #\"^/private$\""
                 " #\"^/private/etc$\""
                 " #\"^/private/etc/\""
                 " #\"^/private/tmp$\""
                 " #\"^/private/tmp/\""
                 " #\"^/private/var$\""
                 " #\"^/private/var/db$\""
                 " #\"^/private/var/folders$\""
                 " #\"^/private/var/lib$\""
                 " #\"^/private/var/log$\""
                 " #\"^/private/var/mysql$\""
                 " #\"^/private/var/run$\""
                 " #\"^/private/var/spool$\""
                 " #\"^/private/var/tmp$\""
                 " #\"^/private/var/tmp/\""
                 " #\"^/usr/bin$\""	/* /usr/bin */
                 " #\"^/usr/bin/\""	/* /usr/bin/... */
                 " #\"^/usr/libexec/cups$\""	/* /usr/libexec/cups */
                 " #\"^/usr/libexec/cups/\""	/* /usr/libexec/cups/... */
                 " #\"^/usr/libexec/fax$\""	/* /usr/libexec/fax */
                 " #\"^/usr/libexec/fax/\""	/* /usr/libexec/fax/... */
                 " #\"^/usr/sbin$\""	/* /usr/sbin */
                 " #\"^/usr/sbin/\""	/* /usr/sbin/... */
		 " #\"^/Library$\""	/* /Library */
		 " #\"^/Library/\""	/* /Library/... */
		 " #\"^/System$\""	/* /System */
		 " #\"^/System/\""	/* /System/... */
		 " #\"^%s/Library$\""	/* RequestRoot/Library */
		 " #\"^%s/Library/\""	/* RequestRoot/Library/... */
		 " #\"^%s$\""		/* ServerBin */
		 " #\"^%s/\""		/* ServerBin/... */
		 " #\"^%s$\""		/* ServerRoot */
		 " #\"^%s/\""		/* ServerRoot/... */
		 "))\n",
		 request, request, bin, bin, root, root);
  if (Sandboxing == CUPSD_SANDBOXING_RELAXED)
  {
    /* Limited write access to /Library/Printers/... */
    cupsFilePuts(fp,
		 "(allow file-write*\n"
		 "  (regex"
		 " #\"^/Library/Printers/.*/\""
		 "))\n");
    cupsFilePrintf(fp,
		   "(deny file-write*\n"
		   "  (regex"
		   " #\"^/Library/Printers/PPDs$\""
		   " #\"^/Library/Printers/PPDs/\""
		   " #\"^/Library/Printers/PPD Plugins$\""
		   " #\"^/Library/Printers/PPD Plugins/\""
		   ")%s)\n", nodebug);
  }
  /* Allow execution of child processes as long as the programs are not in a user directory */
  cupsFilePuts(fp, "(allow process*)\n");
  cupsFilePuts(fp, "(deny process-exec (regex #\"^/Users/\"))\n");
  if (RunUser && getenv("CUPS_TESTROOT"))
  {
    /* Allow source directory access in "make test" environment */
    char	testroot[1024];		/* Root directory of test files */

    cupsd_requote(testroot, getenv("CUPS_TESTROOT"), sizeof(testroot));

    cupsFilePrintf(fp,
		   "(allow file-write* file-read-data file-read-metadata\n"
		   "  (regex"
		   " #\"^%s$\""		/* CUPS_TESTROOT */
		   " #\"^%s/\""		/* CUPS_TESTROOT/... */
		   "))\n",
		   testroot, testroot);
    cupsFilePrintf(fp,
		   "(allow process-exec\n"
		   "  (regex"
		   " #\"^%s/\""		/* CUPS_TESTROOT/... */
		   "))\n",
		   testroot);
    cupsFilePrintf(fp, "(allow sysctl*)\n");
  }
  if (job_id)
  {
    /* Allow job filters to read the current job files... */
    cupsFilePrintf(fp,
                   "(allow file-read-data file-read-metadata\n"
                   "       (regex #\"^%s/([ac]%05d|d%05d-[0-9][0-9][0-9])$\"))\n",
		   request, job_id, job_id);
  }
  else
  {
    /* Allow email notifications from notifiers... */
    cupsFilePuts(fp,
		 "(allow process-exec\n"
		 "  (literal \"/usr/sbin/sendmail\")\n"
		 "  (with no-sandbox))\n");
  }
  /* Allow access to Bluetooth, USB, and notify_post. */
  cupsFilePuts(fp, "(allow iokit*)\n");
  cupsFilePuts(fp, "(allow distributed-notification-post)\n");
  /* Allow outbound networking to local services */
  cupsFilePuts(fp, "(allow network-outbound"
		   "\n       (regex #\"^/private/var/run/\" #\"^/private/tmp/\" #\"^/private/var/tmp/\")");
  for (lis = (cupsd_listener_t *)cupsArrayFirst(Listeners);
       lis;
       lis = (cupsd_listener_t *)cupsArrayNext(Listeners))
  {
    if (httpAddrFamily(&(lis->address)) == AF_LOCAL)
    {
      httpAddrString(&(lis->address), domain, sizeof(domain));
      cupsFilePrintf(fp, "\n       (literal \"%s\")", domain);
    }
  }
  if (allow_networking)
  {
    /* Allow TCP and UDP networking off the machine... */
    cupsFilePuts(fp, "\n       (remote tcp))\n");
    cupsFilePuts(fp, "(allow network-bind)\n"); /* for LPD resvport */
    cupsFilePuts(fp, "(allow network*\n"
		     "       (local udp \"*:*\")\n"
		     "       (remote udp \"*:*\"))\n");

    /* Also allow access to device files... */
    cupsFilePuts(fp, "(allow file-write* file-read-data file-read-metadata file-ioctl\n"
                     "       (regex #\"^/dev/\"))\n");

    /* And allow kernel extensions to be loaded, e.g., SMB */
    cupsFilePuts(fp, "(allow system-kext-load)\n");
  }
  else
  {
    /* Only allow SNMP (UDP) and LPD (TCP) off the machine... */
    cupsFilePuts(fp, ")\n");
    cupsFilePuts(fp, "(allow network-outbound\n"
		     "       (remote udp \"*:161\")\n"
		     "       (remote tcp \"*:515\"))\n");
    cupsFilePuts(fp, "(allow network-inbound\n"
		     "       (local udp \"localhost:*\"))\n");
  }
  cupsFileClose(fp);

  cupsdLogMessage(CUPSD_LOG_DEBUG2, "cupsdCreateProfile(job_id=%d,allow_networking=%d) = \"%s\"", job_id, allow_networking, profile);
  return ((void *)strdup(profile));

#else
  cupsdLogMessage(CUPSD_LOG_DEBUG2, "cupsdCreateProfile(job_id=%d, allow_networking=%d) = NULL", job_id, allow_networking);

  return (NULL);
#endif /* HAVE_SANDBOX_H */
}