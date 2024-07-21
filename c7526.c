queue_action(uschar *id, int action, uschar **argv, int argc, int recipients_arg)
{
int i, j;
BOOL yield = TRUE;
BOOL removed = FALSE;
struct passwd *pw;
uschar *doing = NULL;
uschar *username;
uschar *errmsg;
uschar spoolname[32];

/* Set the global message_id variable, used when re-writing spool files. This
also causes message ids to be added to log messages. */

Ustrcpy(message_id, id);

/* The "actions" that just list the files do not require any locking to be
done. Only admin users may read the spool files. */

if (action >= MSG_SHOW_BODY)
  {
  int fd, i, rc;
  uschar *subdirectory, *suffix;

  if (!f.admin_user)
    {
    printf("Permission denied\n");
    return FALSE;
    }

  if (recipients_arg < argc)
    {
    printf("*** Only one message can be listed at once\n");
    return FALSE;
    }

  if (action == MSG_SHOW_BODY)
    {
    subdirectory = US"input";
    suffix = US"-D";
    }
  else if (action == MSG_SHOW_HEADER)
    {
    subdirectory = US"input";
    suffix = US"-H";
    }
  else
    {
    subdirectory = US"msglog";
    suffix = US"";
    }

  for (i = 0; i < 2; i++)
    {
    message_subdir[0] = split_spool_directory == (i == 0) ? id[5] : 0;
    if ((fd = Uopen(spool_fname(subdirectory, message_subdir, id, suffix),
		    O_RDONLY, 0)) >= 0)
      break;
    if (i == 0)
      continue;

    printf("Failed to open %s file for %s%s: %s\n", subdirectory, id, suffix,
      strerror(errno));
    if (action == MSG_SHOW_LOG && !message_logs)
      printf("(No message logs are being created because the message_logs "
        "option is false.)\n");
    return FALSE;
    }

  while((rc = read(fd, big_buffer, big_buffer_size)) > 0)
    rc = write(fileno(stdout), big_buffer, rc);

  (void)close(fd);
  return TRUE;
  }

/* For actions that actually act, open and lock the data file to ensure that no
other process is working on this message. If the file does not exist, continue
only if the action is remove and the user is an admin user, to allow for
tidying up broken states. */

if ((deliver_datafile = spool_open_datafile(id)) < 0)
  if (errno == ENOENT)
    {
    yield = FALSE;
    printf("Spool data file for %s does not exist\n", id);
    if (action != MSG_REMOVE || !f.admin_user) return FALSE;
    printf("Continuing, to ensure all files removed\n");
    }
  else
    {
    if (errno == 0) printf("Message %s is locked\n", id);
      else printf("Couldn't open spool file for %s: %s\n", id,
        strerror(errno));
    return FALSE;
    }

/* Read the spool header file for the message. Again, continue after an
error only in the case of deleting by an administrator. Setting the third
argument false causes it to look both in the main spool directory and in
the appropriate subdirectory, and set message_subdir according to where it
found the message. */

sprintf(CS spoolname, "%s-H", id);
if (spool_read_header(spoolname, TRUE, FALSE) != spool_read_OK)
  {
  yield = FALSE;
  if (errno != ERRNO_SPOOLFORMAT)
    printf("Spool read error for %s: %s\n", spoolname, strerror(errno));
  else
    printf("Spool format error for %s\n", spoolname);
  if (action != MSG_REMOVE || !f.admin_user)
    {
    (void)close(deliver_datafile);
    deliver_datafile = -1;
    return FALSE;
    }
  printf("Continuing to ensure all files removed\n");
  }

/* Check that the user running this process is entitled to operate on this
message. Only admin users may freeze/thaw, add/cancel recipients, or otherwise
mess about, but the original sender is permitted to remove a message. That's
why we leave this check until after the headers are read. */

if (!f.admin_user && (action != MSG_REMOVE || real_uid != originator_uid))
  {
  printf("Permission denied\n");
  (void)close(deliver_datafile);
  deliver_datafile = -1;
  return FALSE;
  }

/* Set up the user name for logging. */

pw = getpwuid(real_uid);
username = (pw != NULL)?
  US pw->pw_name : string_sprintf("uid %ld", (long int)real_uid);

/* Take the necessary action. */

if (action != MSG_SHOW_COPY) printf("Message %s ", id);

switch(action)
  {
  case MSG_SHOW_COPY:
    {
    transport_ctx tctx = {{0}};
    deliver_in_buffer = store_malloc(DELIVER_IN_BUFFER_SIZE);
    deliver_out_buffer = store_malloc(DELIVER_OUT_BUFFER_SIZE);
    tctx.u.fd = 1;
    transport_write_message(&tctx, 0);
    break;
    }


  case MSG_FREEZE:
  if (f.deliver_freeze)
    {
    yield = FALSE;
    printf("is already frozen\n");
    }
  else
    {
    f.deliver_freeze = TRUE;
    f.deliver_manual_thaw = FALSE;
    deliver_frozen_at = time(NULL);
    if (spool_write_header(id, SW_MODIFYING, &errmsg) >= 0)
      {
      printf("is now frozen\n");
      log_write(0, LOG_MAIN, "frozen by %s", username);
      }
    else
      {
      yield = FALSE;
      printf("could not be frozen: %s\n", errmsg);
      }
    }
  break;


  case MSG_THAW:
  if (!f.deliver_freeze)
    {
    yield = FALSE;
    printf("is not frozen\n");
    }
  else
    {
    f.deliver_freeze = FALSE;
    f.deliver_manual_thaw = TRUE;
    if (spool_write_header(id, SW_MODIFYING, &errmsg) >= 0)
      {
      printf("is no longer frozen\n");
      log_write(0, LOG_MAIN, "unfrozen by %s", username);
      }
    else
      {
      yield = FALSE;
      printf("could not be unfrozen: %s\n", errmsg);
      }
    }
  break;


  /* We must ensure all files are removed from both the input directory
  and the appropriate subdirectory, to clean up cases when there are odd
  files left lying around in odd places. In the normal case message_subdir
  will have been set correctly by spool_read_header, but as this is a rare
  operation, just run everything twice. */

  case MSG_REMOVE:
    {
    uschar suffix[3];

    suffix[0] = '-';
    suffix[2] = 0;
    message_subdir[0] = id[5];

    for (j = 0; j < 2; message_subdir[0] = 0, j++)
      {
      uschar * fname = spool_fname(US"msglog", message_subdir, id, US"");

      DEBUG(D_any) debug_printf(" removing %s", fname);
      if (Uunlink(fname) < 0)
	{
	if (errno != ENOENT)
	  {
	  yield = FALSE;
	  printf("Error while removing %s: %s\n", fname, strerror(errno));
	  }
	else DEBUG(D_any) debug_printf(" (no file)\n");
	}
      else
	{
	removed = TRUE;
	DEBUG(D_any) debug_printf(" (ok)\n");
	}

      for (i = 0; i < 3; i++)
	{
	uschar * fname;

	suffix[1] = (US"DHJ")[i];
	fname = spool_fname(US"input", message_subdir, id, suffix);

	DEBUG(D_any) debug_printf(" removing %s", fname);
	if (Uunlink(fname) < 0)
	  {
	  if (errno != ENOENT)
	    {
	    yield = FALSE;
	    printf("Error while removing %s: %s\n", fname, strerror(errno));
	    }
	  else DEBUG(D_any) debug_printf(" (no file)\n");
	  }
	else
	  {
	  removed = TRUE;
	  DEBUG(D_any) debug_printf(" (done)\n");
	  }
	}
      }

    /* In the common case, the datafile is open (and locked), so give the
    obvious message. Otherwise be more specific. */

    if (deliver_datafile >= 0) printf("has been removed\n");
      else printf("has been removed or did not exist\n");
    if (removed)
      {
      log_write(0, LOG_MAIN, "removed by %s", username);
      log_write(0, LOG_MAIN, "Completed");
      }
    break;
    }


  case MSG_MARK_ALL_DELIVERED:
  for (i = 0; i < recipients_count; i++)
    {
    tree_add_nonrecipient(recipients_list[i].address);
    }
  if (spool_write_header(id, SW_MODIFYING, &errmsg) >= 0)
    {
    printf("has been modified\n");
    for (i = 0; i < recipients_count; i++)
      log_write(0, LOG_MAIN, "address <%s> marked delivered by %s",
        recipients_list[i].address, username);
    }
  else
    {
    yield = FALSE;
    printf("- could not mark all delivered: %s\n", errmsg);
    }
  break;


  case MSG_EDIT_SENDER:
  if (recipients_arg < argc - 1)
    {
    yield = FALSE;
    printf("- only one sender address can be specified\n");
    break;
    }
  doing = US"editing sender";
  /* Fall through */

  case MSG_ADD_RECIPIENT:
  if (doing == NULL) doing = US"adding recipient";
  /* Fall through */

  case MSG_MARK_DELIVERED:
  if (doing == NULL) doing = US"marking as delivered";

  /* Common code for EDIT_SENDER, ADD_RECIPIENT, & MARK_DELIVERED */

  if (recipients_arg >= argc)
    {
    yield = FALSE;
    printf("- error while %s: no address given\n", doing);
    break;
    }

  for (; recipients_arg < argc; recipients_arg++)
    {
    int start, end, domain;
    uschar *errmess;
    uschar *recipient =
      parse_extract_address(argv[recipients_arg], &errmess, &start, &end,
        &domain, (action == MSG_EDIT_SENDER));

    if (recipient == NULL)
      {
      yield = FALSE;
      printf("- error while %s:\n  bad address %s: %s\n",
        doing, argv[recipients_arg], errmess);
      }
    else if (recipient[0] != 0 && domain == 0)
      {
      yield = FALSE;
      printf("- error while %s:\n  bad address %s: "
        "domain missing\n", doing, argv[recipients_arg]);
      }
    else
      {
      if (action == MSG_ADD_RECIPIENT)
        {
#ifdef SUPPORT_I18N
	if (string_is_utf8(recipient)) allow_utf8_domains = message_smtputf8 = TRUE;
#endif
        receive_add_recipient(recipient, -1);
        log_write(0, LOG_MAIN, "recipient <%s> added by %s",
          recipient, username);
        }
      else if (action == MSG_MARK_DELIVERED)
        {
        for (i = 0; i < recipients_count; i++)
          if (Ustrcmp(recipients_list[i].address, recipient) == 0) break;
        if (i >= recipients_count)
          {
          printf("- error while %s:\n  %s is not a recipient:"
            " message not updated\n", doing, recipient);
          yield = FALSE;
          }
        else
          {
          tree_add_nonrecipient(recipients_list[i].address);
          log_write(0, LOG_MAIN, "address <%s> marked delivered by %s",
            recipient, username);
          }
        }
      else  /* MSG_EDIT_SENDER */
        {
#ifdef SUPPORT_I18N
	if (string_is_utf8(recipient)) allow_utf8_domains = message_smtputf8 = TRUE;
#endif
        sender_address = recipient;
        log_write(0, LOG_MAIN, "sender address changed to <%s> by %s",
          recipient, username);
        }
      }
    }

  if (yield)
    if (spool_write_header(id, SW_MODIFYING, &errmsg) >= 0)
      printf("has been modified\n");
    else
      {
      yield = FALSE;
      printf("- while %s: %s\n", doing, errmsg);
      }

  break;
  }

/* Closing the datafile releases the lock and permits other processes
to operate on the message (if it still exists). */

if (deliver_datafile >= 0)
  {
  (void)close(deliver_datafile);
  deliver_datafile = -1;
  }
return yield;
}