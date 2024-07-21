handle_smtp_call(int *listen_sockets, int listen_socket_count,
  int accept_socket, struct sockaddr *accepted)
{
pid_t pid;
union sockaddr_46 interface_sockaddr;
EXIM_SOCKLEN_T ifsize = sizeof(interface_sockaddr);
int dup_accept_socket = -1;
int max_for_this_host = 0;
int wfsize = 0;
int wfptr = 0;
int use_log_write_selector = log_write_selector;
uschar *whofrom = NULL;

void *reset_point = store_get(0);

/* Make the address available in ASCII representation, and also fish out
the remote port. */

sender_host_address = host_ntoa(-1, accepted, NULL, &sender_host_port);
DEBUG(D_any) debug_printf("Connection request from %s port %d\n",
  sender_host_address, sender_host_port);

/* Set up the output stream, check the socket has duplicated, and set up the
input stream. These operations fail only the exceptional circumstances. Note
that never_error() won't use smtp_out if it is NULL. */

smtp_out = fdopen(accept_socket, "wb");
if (smtp_out == NULL)
  {
  never_error(US"daemon: fdopen() for smtp_out failed", US"", errno);
  goto ERROR_RETURN;
  }

dup_accept_socket = dup(accept_socket);
if (dup_accept_socket < 0)
  {
  never_error(US"daemon: couldn't dup socket descriptor",
    US"Connection setup failed", errno);
  goto ERROR_RETURN;
  }

smtp_in = fdopen(dup_accept_socket, "rb");
if (smtp_in == NULL)
  {
  never_error(US"daemon: fdopen() for smtp_in failed",
    US"Connection setup failed", errno);
  goto ERROR_RETURN;
  }

/* Get the data for the local interface address. Panic for most errors, but
"connection reset by peer" just means the connection went away. */

if (getsockname(accept_socket, (struct sockaddr *)(&interface_sockaddr),
     &ifsize) < 0)
  {
  log_write(0, LOG_MAIN | ((errno == ECONNRESET)? 0 : LOG_PANIC),
    "getsockname() failed: %s", strerror(errno));
  smtp_printf("421 Local problem: getsockname() failed; please try again later\r\n");
  goto ERROR_RETURN;
  }

interface_address = host_ntoa(-1, &interface_sockaddr, NULL, &interface_port);
DEBUG(D_interface) debug_printf("interface address=%s port=%d\n",
  interface_address, interface_port);

/* Build a string identifying the remote host and, if requested, the port and
the local interface data. This is for logging; at the end of this function the
memory is reclaimed. */

whofrom = string_append(whofrom, &wfsize, &wfptr, 3, "[", sender_host_address, "]");

if ((log_extra_selector & LX_incoming_port) != 0)
  whofrom = string_append(whofrom, &wfsize, &wfptr, 2, ":", string_sprintf("%d",
    sender_host_port));

if ((log_extra_selector & LX_incoming_interface) != 0)
  whofrom = string_append(whofrom, &wfsize, &wfptr, 4, " I=[",
    interface_address, "]:", string_sprintf("%d", interface_port));

whofrom[wfptr] = 0;    /* Terminate the newly-built string */

/* Check maximum number of connections. We do not check for reserved
connections or unacceptable hosts here. That is done in the subprocess because
it might take some time. */

if (smtp_accept_max > 0 && smtp_accept_count >= smtp_accept_max)
  {
  DEBUG(D_any) debug_printf("rejecting SMTP connection: count=%d max=%d\n",
    smtp_accept_count, smtp_accept_max);
  smtp_printf("421 Too many concurrent SMTP connections; "
    "please try again later.\r\n");
  log_write(L_connection_reject,
            LOG_MAIN, "Connection from %s refused: too many connections",
    whofrom);
  goto ERROR_RETURN;
  }

/* If a load limit above which only reserved hosts are acceptable is defined,
get the load average here, and if there are in fact no reserved hosts, do
the test right away (saves a fork). If there are hosts, do the check in the
subprocess because it might take time. */

if (smtp_load_reserve >= 0)
  {
  load_average = OS_GETLOADAVG();
  if (smtp_reserve_hosts == NULL && load_average > smtp_load_reserve)
    {
    DEBUG(D_any) debug_printf("rejecting SMTP connection: load average = %.2f\n",
      (double)load_average/1000.0);
    smtp_printf("421 Too much load; please try again later.\r\n");
    log_write(L_connection_reject,
              LOG_MAIN, "Connection from %s refused: load average = %.2f",
      whofrom, (double)load_average/1000.0);
    goto ERROR_RETURN;
    }
  }

/* Check that one specific host (strictly, IP address) is not hogging
resources. This is done here to prevent a denial of service attack by someone
forcing you to fork lots of times before denying service. The value of
smtp_accept_max_per_host is a string which is expanded. This makes it possible
to provide host-specific limits according to $sender_host address, but because
this is in the daemon mainline, only fast expansions (such as inline address
checks) should be used. The documentation is full of warnings. */

if (smtp_accept_max_per_host != NULL)
  {
  uschar *expanded = expand_string(smtp_accept_max_per_host);
  if (expanded == NULL)
    {
    if (!expand_string_forcedfail)
      log_write(0, LOG_MAIN|LOG_PANIC, "expansion of smtp_accept_max_per_host "
        "failed for %s: %s", whofrom, expand_string_message);
    }
  /* For speed, interpret a decimal number inline here */
  else
    {
    uschar *s = expanded;
    while (isdigit(*s))
      max_for_this_host = max_for_this_host * 10 + *s++ - '0';
    if (*s != 0)
      log_write(0, LOG_MAIN|LOG_PANIC, "expansion of smtp_accept_max_per_host "
        "for %s contains non-digit: %s", whofrom, expanded);
    }
  }

/* If we have fewer connections than max_for_this_host, we can skip the tedious
per host_address checks. Note that at this stage smtp_accept_count contains the
count of *other* connections, not including this one. */

if ((max_for_this_host > 0) &&
    (smtp_accept_count >= max_for_this_host))
  {
  int i;
  int host_accept_count = 0;
  int other_host_count = 0;    /* keep a count of non matches to optimise */

  for (i = 0; i < smtp_accept_max; ++i)
    {
    if (smtp_slots[i].host_address != NULL)
      {
      if (Ustrcmp(sender_host_address, smtp_slots[i].host_address) == 0)
       host_accept_count++;
      else
       other_host_count++;

      /* Testing all these strings is expensive - see if we can drop out
      early, either by hitting the target, or finding there are not enough
      connections left to make the target. */

      if ((host_accept_count >= max_for_this_host) ||
         ((smtp_accept_count - other_host_count) < max_for_this_host))
       break;
      }
    }

  if (host_accept_count >= max_for_this_host)
    {
    DEBUG(D_any) debug_printf("rejecting SMTP connection: too many from this "
      "IP address: count=%d max=%d\n",
      host_accept_count, max_for_this_host);
    smtp_printf("421 Too many concurrent SMTP connections "
      "from this IP address; please try again later.\r\n");
    log_write(L_connection_reject,
              LOG_MAIN, "Connection from %s refused: too many connections "
      "from that IP address", whofrom);
    goto ERROR_RETURN;
    }
  }

/* OK, the connection count checks have been passed. Before we can fork the
accepting process, we must first log the connection if requested. This logging
used to happen in the subprocess, but doing that means that the value of
smtp_accept_count can be out of step by the time it is logged. So we have to do
the logging here and accept the performance cost. Note that smtp_accept_count
hasn't yet been incremented to take account of this connection.

In order to minimize the cost (because this is going to happen for every
connection), do a preliminary selector test here. This saves ploughing through
the generalized logging code each time when the selector is false. If the
selector is set, check whether the host is on the list for logging. If not,
arrange to unset the selector in the subprocess. */

if ((log_write_selector & L_smtp_connection) != 0)
  {
  uschar *list = hosts_connection_nolog;
  if (list != NULL && verify_check_host(&list) == OK)
    use_log_write_selector &= ~L_smtp_connection;
  else
    log_write(L_smtp_connection, LOG_MAIN, "SMTP connection from %s "
      "(TCP/IP connection count = %d)", whofrom, smtp_accept_count + 1);
  }

/* Now we can fork the accepting process; do a lookup tidy, just in case any
expansion above did a lookup. */

search_tidyup();
pid = fork();

/* Handle the child process */

if (pid == 0)
  {
  int i;
  int queue_only_reason = 0;
  int old_pool = store_pool;
  int save_debug_selector = debug_selector;
  BOOL local_queue_only;
  BOOL session_local_queue_only;
  #ifdef SA_NOCLDWAIT
  struct sigaction act;
  #endif

  smtp_accept_count++;    /* So that it includes this process */

  /* May have been modified for the subprocess */

  log_write_selector = use_log_write_selector;

  /* Get the local interface address into permanent store */

  store_pool = POOL_PERM;
  interface_address = string_copy(interface_address);
  store_pool = old_pool;

  /* Check for a tls-on-connect port */

  if (host_is_tls_on_connect_port(interface_port)) tls_on_connect = TRUE;

  /* Expand smtp_active_hostname if required. We do not do this any earlier,
  because it may depend on the local interface address (indeed, that is most
  likely what it depends on.) */

  smtp_active_hostname = primary_hostname;
  if (raw_active_hostname != NULL)
    {
    uschar *nah = expand_string(raw_active_hostname);
    if (nah == NULL)
      {
      if (!expand_string_forcedfail)
        {
        log_write(0, LOG_MAIN|LOG_PANIC, "failed to expand \"%s\" "
          "(smtp_active_hostname): %s", raw_active_hostname,
          expand_string_message);
        smtp_printf("421 Local configuration error; "
          "please try again later.\r\n");
        mac_smtp_fflush();
        search_tidyup();
        _exit(EXIT_FAILURE);
        }
      }
    else if (nah[0] != 0) smtp_active_hostname = nah;
    }

  /* Initialize the queueing flags */

  queue_check_only();
  session_local_queue_only = queue_only;

  /* Close the listening sockets, and set the SIGCHLD handler to SIG_IGN.
  We also attempt to set things up so that children are automatically reaped,
  but just in case this isn't available, there's a paranoid waitpid() in the
  loop too (except for systems where we are sure it isn't needed). See the more
  extensive comment before the reception loop in exim.c for a fuller
  explanation of this logic. */

  for (i = 0; i < listen_socket_count; i++) (void)close(listen_sockets[i]);

  #ifdef SA_NOCLDWAIT
  act.sa_handler = SIG_IGN;
  sigemptyset(&(act.sa_mask));
  act.sa_flags = SA_NOCLDWAIT;
  sigaction(SIGCHLD, &act, NULL);
  #else
  signal(SIGCHLD, SIG_IGN);
  #endif

  /* Attempt to get an id from the sending machine via the RFC 1413
  protocol. We do this in the sub-process in order not to hold up the
  main process if there is any delay. Then set up the fullhost information
  in case there is no HELO/EHLO.

  If debugging is enabled only for the daemon, we must turn if off while
  finding the id, but turn it on again afterwards so that information about the
  incoming connection is output. */

  if (debug_daemon) debug_selector = 0;
  verify_get_ident(IDENT_PORT);
  host_build_sender_fullhost();
  debug_selector = save_debug_selector;

  DEBUG(D_any)
    debug_printf("Process %d is handling incoming connection from %s\n",
      (int)getpid(), sender_fullhost);

  /* Now disable debugging permanently if it's required only for the daemon
  process. */

  if (debug_daemon) debug_selector = 0;

  /* If there are too many child processes for immediate delivery,
  set the session_local_queue_only flag, which is initialized from the
  configured value and may therefore already be TRUE. Leave logging
  till later so it will have a message id attached. Note that there is no
  possibility of re-calculating this per-message, because the value of
  smtp_accept_count does not change in this subprocess. */

  if (smtp_accept_queue > 0 && smtp_accept_count > smtp_accept_queue)
    {
    session_local_queue_only = TRUE;
    queue_only_reason = 1;
    }

  /* Handle the start of the SMTP session, then loop, accepting incoming
  messages from the SMTP connection. The end will come at the QUIT command,
  when smtp_setup_msg() returns 0. A break in the connection causes the
  process to die (see accept.c).

  NOTE: We do *not* call smtp_log_no_mail() if smtp_start_session() fails,
  because a log line has already been written for all its failure exists
  (usually "connection refused: <reason>") and writing another one is
  unnecessary clutter. */

  if (!smtp_start_session())
    {
    mac_smtp_fflush();
    search_tidyup();
    _exit(EXIT_SUCCESS);
    }

  for (;;)
    {
    int rc;
    message_id[0] = 0;            /* Clear out any previous message_id */
    reset_point = store_get(0);   /* Save current store high water point */

    DEBUG(D_any)
      debug_printf("Process %d is ready for new message\n", (int)getpid());

    /* Smtp_setup_msg() returns 0 on QUIT or if the call is from an
    unacceptable host or if an ACL "drop" command was triggered, -1 on
    connection lost, and +1 on validly reaching DATA. Receive_msg() almost
    always returns TRUE when smtp_input is true; just retry if no message was
    accepted (can happen for invalid message parameters). However, it can yield
    FALSE if the connection was forcibly dropped by the DATA ACL. */

    if ((rc = smtp_setup_msg()) > 0)
      {
      BOOL ok = receive_msg(FALSE);
      search_tidyup();                    /* Close cached databases */
      if (!ok)                            /* Connection was dropped */
        {
        mac_smtp_fflush();
        smtp_log_no_mail();               /* Log no mail if configured */
        _exit(EXIT_SUCCESS);
        }
      if (message_id[0] == 0) continue;   /* No message was accepted */
      }
    else
      {
      mac_smtp_fflush();
      search_tidyup();
      smtp_log_no_mail();                 /* Log no mail if configured */
      _exit((rc == 0)? EXIT_SUCCESS : EXIT_FAILURE);
      }

    /* Show the recipients when debugging */

    DEBUG(D_receive)
      {
      int i;
      if (sender_address != NULL)
        debug_printf("Sender: %s\n", sender_address);
      if (recipients_list != NULL)
        {
        debug_printf("Recipients:\n");
        for (i = 0; i < recipients_count; i++)
          debug_printf("  %s\n", recipients_list[i].address);
        }
      }

    /* A message has been accepted. Clean up any previous delivery processes
    that have completed and are defunct, on systems where they don't go away
    by themselves (see comments when setting SIG_IGN above). On such systems
    (if any) these delivery processes hang around after termination until
    the next message is received. */

    #ifndef SIG_IGN_WORKS
    while (waitpid(-1, NULL, WNOHANG) > 0);
    #endif

    /* Reclaim up the store used in accepting this message */

    store_reset(reset_point);

    /* If queue_only is set or if there are too many incoming connections in
    existence, session_local_queue_only will be TRUE. If it is not, check
    whether we have received too many messages in this session for immediate
    delivery. */

    if (!session_local_queue_only &&
        smtp_accept_queue_per_connection > 0 &&
        receive_messagecount > smtp_accept_queue_per_connection)
      {
      session_local_queue_only = TRUE;
      queue_only_reason = 2;
      }

    /* Initialize local_queue_only from session_local_queue_only. If it is not
    true, and queue_only_load is set, check that the load average is below it.
    If local_queue_only is set by this means, we also set if for the session if
    queue_only_load_latch is true (the default). This means that, once set,
    local_queue_only remains set for any subsequent messages on the same SMTP
    connection. This is a deliberate choice; even though the load average may
    fall, it doesn't seem right to deliver later messages on the same call when
    not delivering earlier ones. However, the are special circumstances such as
    very long-lived connections from scanning appliances where this is not the
    best strategy. In such cases, queue_only_load_latch should be set false. */

    local_queue_only = session_local_queue_only;
    if (!local_queue_only && queue_only_load >= 0)
      {
      local_queue_only = (load_average = OS_GETLOADAVG()) > queue_only_load;
      if (local_queue_only)
        {
        queue_only_reason = 3;
        if (queue_only_load_latch) session_local_queue_only = TRUE;
        }
      }

    /* Log the queueing here, when it will get a message id attached, but
    not if queue_only is set (case 0). */

    if (local_queue_only) switch(queue_only_reason)
      {
      case 1:
      log_write(L_delay_delivery,
                LOG_MAIN, "no immediate delivery: too many connections "
                "(%d, max %d)", smtp_accept_count, smtp_accept_queue);
      break;

      case 2:
      log_write(L_delay_delivery,
                LOG_MAIN, "no immediate delivery: more than %d messages "
                "received in one connection", smtp_accept_queue_per_connection);
      break;

      case 3:
      log_write(L_delay_delivery,
                LOG_MAIN, "no immediate delivery: load average %.2f",
                (double)load_average/1000.0);
      break;
      }

    /* If a delivery attempt is required, spin off a new process to handle it.
    If we are not root, we have to re-exec exim unless deliveries are being
    done unprivileged. */

    else if (!queue_only_policy && !deliver_freeze)
      {
      pid_t dpid;

      /* Before forking, ensure that the C output buffer is flushed. Otherwise
      anything that it in it will get duplicated, leading to duplicate copies
      of the pending output. */

      mac_smtp_fflush();

      if ((dpid = fork()) == 0)
        {
        (void)fclose(smtp_in);
        (void)fclose(smtp_out);

        /* Don't ever molest the parent's SSL connection, but do clean up
        the data structures if necessary. */

        #ifdef SUPPORT_TLS
        tls_close(FALSE);
        #endif

        /* Reset SIGHUP and SIGCHLD in the child in both cases. */

        signal(SIGHUP,  SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        if (geteuid() != root_uid && !deliver_drop_privilege)
          {
          signal(SIGALRM, SIG_DFL);
          (void)child_exec_exim(CEE_EXEC_PANIC, FALSE, NULL, FALSE, 2, US"-Mc",
            message_id);
          /* Control does not return here. */
          }

        /* No need to re-exec; SIGALRM remains set to the default handler */

        (void)deliver_message(message_id, FALSE, FALSE);
        search_tidyup();
        _exit(EXIT_SUCCESS);
        }

      if (dpid > 0)
        {
        DEBUG(D_any) debug_printf("forked delivery process %d\n", (int)dpid);
        }
      else
        {
        log_write(0, LOG_MAIN|LOG_PANIC, "daemon: delivery process fork "
          "failed: %s", strerror(errno));
        }
      }
    }
  }


/* Carrying on in the parent daemon process... Can't do much if the fork
failed. Otherwise, keep count of the number of accepting processes and
remember the pid for ticking off when the child completes. */

if (pid < 0)
  {
  never_error(US"daemon: accept process fork failed", US"Fork failed", errno);
  }
else
  {
  int i;
  for (i = 0; i < smtp_accept_max; ++i)
    {
    if (smtp_slots[i].pid <= 0)
      {
      smtp_slots[i].pid = pid;
      if (smtp_accept_max_per_host != NULL)
        smtp_slots[i].host_address = string_copy_malloc(sender_host_address);
      smtp_accept_count++;
      break;
      }
    }
  DEBUG(D_any) debug_printf("%d SMTP accept process%s running\n",
    smtp_accept_count, (smtp_accept_count == 1)? "" : "es");
  }

/* Get here via goto in error cases */

ERROR_RETURN:

/* Close the streams associated with the socket which will also close the
socket fds in this process. We can't do anything if fclose() fails, but
logging brings it to someone's attention. However, "connection reset by peer"
isn't really a problem, so skip that one. On Solaris, a dropped connection can
manifest itself as a broken pipe, so drop that one too. If the streams don't
exist, something went wrong while setting things up. Make sure the socket
descriptors are closed, in order to drop the connection. */

if (smtp_out != NULL)
  {
  if (fclose(smtp_out) != 0 && errno != ECONNRESET && errno != EPIPE)
    log_write(0, LOG_MAIN|LOG_PANIC, "daemon: fclose(smtp_out) failed: %s",
      strerror(errno));
  smtp_out = NULL;
  }
else (void)close(accept_socket);

if (smtp_in != NULL)
  {
  if (fclose(smtp_in) != 0 && errno != ECONNRESET && errno != EPIPE)
    log_write(0, LOG_MAIN|LOG_PANIC, "daemon: fclose(smtp_in) failed: %s",
      strerror(errno));
  smtp_in = NULL;
  }
else (void)close(dup_accept_socket);

/* Release any store used in this process, including the store used for holding
the incoming host address and an expanded active_hostname. */

store_reset(reset_point);
sender_host_address = NULL;
}