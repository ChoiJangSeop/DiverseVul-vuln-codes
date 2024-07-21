start_job(cupsd_job_t     *job,		/* I - Job ID */
          cupsd_printer_t *printer)	/* I - Printer to print job */
{
  const char	*filename;		/* Support filename */
  ipp_attribute_t *cancel_after = ippFindAttribute(job->attrs,
						   "job-cancel-after",
						   IPP_TAG_INTEGER);
					/* job-cancel-after attribute */


  cupsdLogMessage(CUPSD_LOG_DEBUG2, "start_job(job=%p(%d), printer=%p(%s))",
                  job, job->id, printer, printer->name);

 /*
  * Make sure we have some files around before we try to print...
  */

  if (job->num_files == 0)
  {
    ippSetString(job->attrs, &job->reasons, 0, "aborted-by-system");
    cupsdSetJobState(job, IPP_JOB_ABORTED, CUPSD_JOB_DEFAULT,
                     "Aborting job because it has no files.");
    return;
  }

 /*
  * Update the printer and job state to "processing"...
  */

  if (!cupsdLoadJob(job))
    return;

  if (!job->printer_message)
    job->printer_message = ippFindAttribute(job->attrs,
                                            "job-printer-state-message",
                                            IPP_TAG_TEXT);
  if (job->printer_message)
    ippSetString(job->attrs, &job->printer_message, 0, "");

  ippSetString(job->attrs, &job->reasons, 0, "job-printing");
  cupsdSetJobState(job, IPP_JOB_PROCESSING, CUPSD_JOB_DEFAULT, NULL);
  cupsdSetPrinterState(printer, IPP_PRINTER_PROCESSING, 0);
  cupsdSetPrinterReasons(printer, "-cups-remote-pending,"
				  "cups-remote-pending-held,"
				  "cups-remote-processing,"
				  "cups-remote-stopped,"
				  "cups-remote-canceled,"
				  "cups-remote-aborted,"
				  "cups-remote-completed");

  job->cost         = 0;
  job->current_file = 0;
  job->file_time    = 0;
  job->history_time = 0;
  job->progress     = 0;
  job->printer      = printer;
  printer->job      = job;

  if (cancel_after)
    job->cancel_time = time(NULL) + ippGetInteger(cancel_after, 0);
  else if (MaxJobTime > 0)
    job->cancel_time = time(NULL) + MaxJobTime;
  else
    job->cancel_time = 0;

 /*
  * Check for support files...
  */

  cupsdSetPrinterReasons(job->printer, "-cups-missing-filter-warning,"
			               "cups-insecure-filter-warning");

  if (printer->pc)
  {
    for (filename = (const char *)cupsArrayFirst(printer->pc->support_files);
         filename;
         filename = (const char *)cupsArrayNext(printer->pc->support_files))
    {
      if (_cupsFileCheck(filename, _CUPS_FILE_CHECK_FILE, !RunUser,
			 cupsdLogFCMessage, printer))
        break;
    }
  }

 /*
  * Setup the last exit status and security profiles...
  */

  job->status   = 0;
  job->profile  = cupsdCreateProfile(job->id, 0);
  job->bprofile = cupsdCreateProfile(job->id, 1);

 /*
  * Create the status pipes and buffer...
  */

  if (cupsdOpenPipe(job->status_pipes))
  {
    cupsdLogJob(job, CUPSD_LOG_DEBUG,
		"Unable to create job status pipes - %s.", strerror(errno));

    cupsdSetJobState(job, IPP_JOB_STOPPED, CUPSD_JOB_DEFAULT,
		     "Job stopped because the scheduler could not create the "
		     "job status pipes.");

    cupsdDestroyProfile(job->profile);
    job->profile = NULL;
    cupsdDestroyProfile(job->bprofile);
    job->bprofile = NULL;
    return;
  }

  job->status_buffer = cupsdStatBufNew(job->status_pipes[0], NULL);
  job->status_level  = CUPSD_LOG_INFO;

 /*
  * Create the backchannel pipes and make them non-blocking...
  */

  if (cupsdOpenPipe(job->back_pipes))
  {
    cupsdLogJob(job, CUPSD_LOG_DEBUG,
		"Unable to create back-channel pipes - %s.", strerror(errno));

    cupsdSetJobState(job, IPP_JOB_STOPPED, CUPSD_JOB_DEFAULT,
		     "Job stopped because the scheduler could not create the "
		     "back-channel pipes.");

    cupsdClosePipe(job->status_pipes);
    cupsdStatBufDelete(job->status_buffer);
    job->status_buffer = NULL;

    cupsdDestroyProfile(job->profile);
    job->profile = NULL;
    cupsdDestroyProfile(job->bprofile);
    job->bprofile = NULL;
    return;
  }

  fcntl(job->back_pipes[0], F_SETFL,
	fcntl(job->back_pipes[0], F_GETFL) | O_NONBLOCK);
  fcntl(job->back_pipes[1], F_SETFL,
	fcntl(job->back_pipes[1], F_GETFL) | O_NONBLOCK);

 /*
  * Create the side-channel pipes and make them non-blocking...
  */

  if (socketpair(AF_LOCAL, SOCK_STREAM, 0, job->side_pipes))
  {
    cupsdLogJob(job, CUPSD_LOG_DEBUG,
		"Unable to create side-channel pipes - %s.", strerror(errno));

    cupsdSetJobState(job, IPP_JOB_STOPPED, CUPSD_JOB_DEFAULT,
		     "Job stopped because the scheduler could not create the "
		     "side-channel pipes.");

    cupsdClosePipe(job->back_pipes);

    cupsdClosePipe(job->status_pipes);
    cupsdStatBufDelete(job->status_buffer);
    job->status_buffer = NULL;

    cupsdDestroyProfile(job->profile);
    job->profile = NULL;
    cupsdDestroyProfile(job->bprofile);
    job->bprofile = NULL;
    return;
  }

  fcntl(job->side_pipes[0], F_SETFL,
	fcntl(job->side_pipes[0], F_GETFL) | O_NONBLOCK);
  fcntl(job->side_pipes[1], F_SETFL,
	fcntl(job->side_pipes[1], F_GETFL) | O_NONBLOCK);

  fcntl(job->side_pipes[0], F_SETFD,
	fcntl(job->side_pipes[0], F_GETFD) | FD_CLOEXEC);
  fcntl(job->side_pipes[1], F_SETFD,
	fcntl(job->side_pipes[1], F_GETFD) | FD_CLOEXEC);

 /*
  * Now start the first file in the job...
  */

  cupsdContinueJob(job);
}