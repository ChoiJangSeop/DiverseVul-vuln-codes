handle_ending_processes(void)
{
int status;
pid_t pid;

while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
  {
  int i;
  DEBUG(D_any) debug_printf("child %d ended: status=0x%x\n", (int)pid,
    status);

  /* If it's a listening daemon for which we are keeping track of individual
  subprocesses, deal with an accepting process that has terminated. */

  if (smtp_slots != NULL)
    {
    for (i = 0; i < smtp_accept_max; i++)
      {
      if (smtp_slots[i].pid == pid)
        {
        if (smtp_slots[i].host_address != NULL)
          store_free(smtp_slots[i].host_address);
        smtp_slots[i] = empty_smtp_slot;
        if (--smtp_accept_count < 0) smtp_accept_count = 0;
        DEBUG(D_any) debug_printf("%d SMTP accept process%s now running\n",
          smtp_accept_count, (smtp_accept_count == 1)? "" : "es");
        break;
        }
      }
    if (i < smtp_accept_max) continue;  /* Found an accepting process */
    }

  /* If it wasn't an accepting process, see if it was a queue-runner
  process that we are tracking. */

  if (queue_pid_slots != NULL)
    {
    for (i = 0; i < queue_run_max; i++)
      {
      if (queue_pid_slots[i] == pid)
        {
        queue_pid_slots[i] = 0;
        if (--queue_run_count < 0) queue_run_count = 0;
        DEBUG(D_any) debug_printf("%d queue-runner process%s now running\n",
          queue_run_count, (queue_run_count == 1)? "" : "es");
        break;
        }
      }
    }
  }
}