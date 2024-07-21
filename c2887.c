CloudInitSetup(const char *tmpDirPath)
{
   int deployStatus = DEPLOY_ERROR;
   const char *cloudInitTmpDirPath = "/var/run/vmware-imc";
   int forkExecResult;
   char command[1024];
   Bool cloudInitTmpDirCreated = FALSE;

   snprintf(command, sizeof(command),
            "/bin/mkdir -p %s", cloudInitTmpDirPath);
   command[sizeof(command) - 1] = '\0';

   forkExecResult = ForkExecAndWaitCommand(command);
   if (forkExecResult != 0) {
      SetDeployError("Error creating %s dir: %s",
                     cloudInitTmpDirPath,
                     strerror(errno));
      goto done;
   }

   cloudInitTmpDirCreated = TRUE;

   snprintf(command, sizeof(command),
            "/bin/rm -f %s/cust.cfg %s/nics.txt",
            cloudInitTmpDirPath, cloudInitTmpDirPath);
   command[sizeof(command) - 1] = '\0';

   forkExecResult = ForkExecAndWaitCommand(command);

   snprintf(command, sizeof(command),
            "/bin/cp %s/cust.cfg %s/cust.cfg",
            tmpDirPath, cloudInitTmpDirPath);
   command[sizeof(command) - 1] = '\0';

   forkExecResult = ForkExecAndWaitCommand(command);

   if (forkExecResult != 0) {
      SetDeployError("Error copying cust.cfg file: %s",
                     strerror(errno));
      goto done;
   }

   snprintf(command, sizeof(command),
            "/usr/bin/test -f %s/nics.txt", tmpDirPath);
   command[sizeof(command) - 1] = '\0';

   forkExecResult = ForkExecAndWaitCommand(command);

   /*
    * /usr/bin/test -f returns 0 if the file exists
    * non zero is returned if the file does not exist.
    * We need to copy the nics.txt only if it exists.
    */
   if (forkExecResult == 0) {
      snprintf(command, sizeof(command),
               "/bin/cp %s/nics.txt %s/nics.txt",
               tmpDirPath, cloudInitTmpDirPath);
      command[sizeof(command) - 1] = '\0';

      forkExecResult = ForkExecAndWaitCommand(command);
      if (forkExecResult != 0) {
         SetDeployError("Error copying nics.txt file: %s",
                        strerror(errno));
         goto done;
      }
   }

   deployStatus = DEPLOY_SUCCESS;

done:
   if (DEPLOY_SUCCESS == deployStatus) {
      TransitionState(INPROGRESS, DONE);
   } else {
      if (cloudInitTmpDirCreated) {
         snprintf(command, sizeof(command),
                  "/bin/rm -rf %s",
                  cloudInitTmpDirPath);
         command[sizeof(command) - 1] = '\0';
         ForkExecAndWaitCommand(command);
      }
      sLog(log_error, "Setting generic error status in vmx. \n");
      SetCustomizationStatusInVmx(TOOLSDEPLOYPKG_RUNNING,
                                  GUESTCUST_EVENT_CUSTOMIZE_FAILED,
                                  NULL);
      TransitionState(INPROGRESS, ERRORED);
   }

   return deployStatus;
}