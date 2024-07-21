cupsdStartServer(void)
{
 /*
  * Start color management (as needed)...
  */

  cupsdStartColor();

 /*
  * Create the default security profile...
  */

  DefaultProfile = cupsdCreateProfile(0, 1);

 /*
  * Startup all the networking stuff...
  */

  cupsdStartListening();
  cupsdStartBrowsing();

 /*
  * Create a pipe for CGI processes...
  */

  if (cupsdOpenPipe(CGIPipes))
    cupsdLogMessage(CUPSD_LOG_ERROR,
                    "cupsdStartServer: Unable to create pipes for CGI status!");
  else
  {
    CGIStatusBuffer = cupsdStatBufNew(CGIPipes[0], "[CGI]");

    cupsdAddSelect(CGIPipes[0], (cupsd_selfunc_t)cupsdUpdateCGI, NULL, NULL);
  }

 /*
  * Mark that the server has started and printers and jobs may be changed...
  */

  LastEvent = CUPSD_EVENT_PRINTER_CHANGED | CUPSD_EVENT_JOB_STATE_CHANGED |
              CUPSD_EVENT_SERVER_STARTED;
  started   = 1;

  cupsdSetBusyState(0);
}