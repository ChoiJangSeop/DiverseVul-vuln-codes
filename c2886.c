Deploy(const char* packageName)
{
   int deployStatus = DEPLOY_SUCCESS;
   char* command = NULL;
   int deploymentResult = 0;
   char *nics;
   char* cleanupCommand;
   uint8 archiveType;
   uint8 flags;
   bool forceSkipReboot = false;
   Bool cloudInitEnabled = FALSE;
   const char *cloudInitConfigFilePath = "/etc/cloud/cloud.cfg";
   char cloudCommand[1024];
   int forkExecResult;

   TransitionState(NULL, INPROGRESS);

   // Notify the vpx of customization in-progress state
   SetCustomizationStatusInVmx(TOOLSDEPLOYPKG_RUNNING,
                               TOOLSDEPLOYPKG_ERROR_SUCCESS,
                               NULL);

   sLog(log_info, "Reading cabinet file %s. \n", packageName);

   // Get the command to execute
   if (!GetPackageInfo(packageName, &command, &archiveType, &flags)) {
      SetDeployError("Error extracting package header information. (%s)",
                     GetDeployError());
      return DEPLOY_ERROR;
   }

   // Print the header command
#ifdef VMX86_DEBUG
   sLog(log_debug, "Header command: %s \n ", command);
#endif

   // create the destination directory
   if (!CreateDir(EXTRACTPATH "/")) {
      free(command);
      return DEPLOY_ERROR;
   }

   if (archiveType == VMWAREDEPLOYPKG_PAYLOAD_TYPE_CAB) {
      if (!ExtractCabPackage(packageName, EXTRACTPATH)) {
         free(command);
         return DEPLOY_ERROR;
      }
   } else if (archiveType == VMWAREDEPLOYPKG_PAYLOAD_TYPE_ZIP) {
      if (!ExtractZipPackage(packageName, EXTRACTPATH)) {
         free(command);
         return DEPLOY_ERROR;
      }
   }

   // check if cloud-init installed
   snprintf(cloudCommand, sizeof(cloudCommand),
            "/usr/bin/cloud-init -h");
   cloudCommand[sizeof(cloudCommand) - 1] = '\0';
   forkExecResult = ForkExecAndWaitCommand(cloudCommand);
   // if cloud-init is installed, check if it is enabled
   if (forkExecResult == 0 && IsCloudInitEnabled(cloudInitConfigFilePath)) {
      cloudInitEnabled = TRUE;
      sSkipReboot = TRUE;
      free(command);
      deployStatus =  CloudInitSetup(EXTRACTPATH);
   } else {
      // Run the deployment command
      sLog(log_info, "Launching deployment %s.  \n", command);
      deploymentResult = ForkExecAndWaitCommand(command);
      free(command);

      if (deploymentResult != 0) {
         sLog(log_error, "Customization process returned with error. \n");
         sLog(log_debug, "Deployment result = %d \n", deploymentResult);

         if (deploymentResult == CUST_NETWORK_ERROR ||
             deploymentResult == CUST_NIC_ERROR) {
            sLog(log_info, "Setting network error status in vmx. \n");
            SetCustomizationStatusInVmx(TOOLSDEPLOYPKG_RUNNING,
                                        GUESTCUST_EVENT_NETWORK_SETUP_FAILED,
                                        NULL);
         } else {
            sLog(log_info, "Setting generic error status in vmx. \n");
            SetCustomizationStatusInVmx(TOOLSDEPLOYPKG_RUNNING,
                                        GUESTCUST_EVENT_CUSTOMIZE_FAILED,
                                        NULL);
         }

         TransitionState(INPROGRESS, ERRORED);

         deployStatus = DEPLOY_ERROR;
         SetDeployError("Deployment failed. "
                        "The forked off process returned error code.");
         sLog(log_error, "Deployment failed. "
                         "The forked off process returned error code. \n");
      } else {
         SetCustomizationStatusInVmx(TOOLSDEPLOYPKG_DONE,
                                     TOOLSDEPLOYPKG_ERROR_SUCCESS,
                                     NULL);

         TransitionState(INPROGRESS, DONE);

         deployStatus = DEPLOY_SUCCESS;
         sLog(log_info, "Deployment succeded. \n");
      }
   }

   if (!cloudInitEnabled || DEPLOY_SUCCESS != deployStatus) {
      /*
       * Read in nics to enable from the nics.txt file. We do it irrespective
       * of the sucess/failure of the customization so that at the end of it we
       * always have nics enabled.
       */
      nics = GetNicsToEnable(EXTRACTPATH);
      if (nics) {
         // XXX: Sleep before the last SetCustomizationStatusInVmx
         //      This is a temporary-hack for PR 422790
         sleep(5);
         sLog(log_info, "Wait before set enable-nics stats in vmx.\n");

         TryToEnableNics(nics);

         free(nics);
      } else {
         sLog(log_info, "No nics to enable.\n");
      }
   }

   cleanupCommand = malloc(strlen(CLEANUPCMD) + strlen(CLEANUPPATH) + 1);
   if (!cleanupCommand) {
      SetDeployError("Error allocating memory.");
      return DEPLOY_ERROR;
   }

   strcpy(cleanupCommand, CLEANUPCMD);
   strcat(cleanupCommand, CLEANUPPATH);

   sLog(log_info, "Launching cleanup. \n");
   if (ForkExecAndWaitCommand(cleanupCommand) != 0) {
      sLog(log_warning, "Error while clean up. Error removing directory %s. (%s)",
           EXTRACTPATH, strerror (errno));
      //TODO: What should be done if cleanup fails ??
   }
   free (cleanupCommand);

   if (flags & VMWAREDEPLOYPKG_HEADER_FLAGS_SKIP_REBOOT) {
      forceSkipReboot = true;
   }
   sLog(log_info,
        "sSkipReboot: %s, forceSkipReboot %s\n",
        sSkipReboot ? "true" : "false",
        forceSkipReboot ? "true" : "false");
   sSkipReboot |= forceSkipReboot;

   //Reset the guest OS
   if (!sSkipReboot && !deploymentResult) {
      pid_t pid = fork();
      if (pid == -1) {
         sLog(log_error, "Failed to fork: %s", strerror(errno));
      } else if (pid == 0) {
         // We're in the child

         // Repeatedly try to reboot to workaround PR 530641 where
         // telinit 6 is overwritten by a telinit 2
         int rebootComandResult = 0;
         do {
            sLog(log_info, "Rebooting\n");
            rebootComandResult = ForkExecAndWaitCommand("/sbin/telinit 6");
            sleep(1);
         } while (rebootComandResult == 0);
         sLog(log_error, "telinit returned error %d\n", rebootComandResult);

         exit (127);
      }
   }

   return deployStatus;
}