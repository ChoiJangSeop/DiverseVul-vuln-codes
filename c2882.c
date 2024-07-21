Deploy(const char* packageName)
{
   int deployStatus = DEPLOY_SUCCESS;
   char* command = NULL;
   int deploymentResult;
   char *nics;
   char* cleanupCommand;
   uint8 archiveType;

   // Move to IN PROGRESS state
   TransitionState(NULL, INPROGRESS);

   // Notify the vpx of customization in-progress state
   SetCustomizationStatusInVmx(TOOLSDEPLOYPKG_RUNNING,
                               TOOLSDEPLOYPKG_ERROR_SUCCESS,
                               NULL);

   sLog(log_info, "Reading cabinet file %s. \n", packageName);

   // Get the command to execute
   if (!GetPackageInfo(packageName, &command, &archiveType)) {
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

   // Run the deployment command
   sLog(log_info, "Launching deployment %s. \n", command);
   deploymentResult = ForkExecAndWaitCommand(command);
   free (command);

   if (deploymentResult != 0) {
      sLog(log_error, "Customization process returned with error. \n");
      sLog(log_debug, "Deployment result = %d \n", deploymentResult);

      if (deploymentResult == CUST_NETWORK_ERROR || deploymentResult == CUST_NIC_ERROR) {
         // Network specific error in the guest
         sLog(log_info, "Setting network error status in vmx. \n");
         SetCustomizationStatusInVmx(TOOLSDEPLOYPKG_RUNNING,
                                     GUESTCUST_EVENT_NETWORK_SETUP_FAILED,
                                     NULL);
      } else {
         // Generic error in the guest
         sLog(log_info, "Setting generic error status in vmx. \n");
         SetCustomizationStatusInVmx(TOOLSDEPLOYPKG_RUNNING,
                                     GUESTCUST_EVENT_CUSTOMIZE_FAILED,
                                     NULL);
      }

      // Move to ERROR state
      TransitionState(INPROGRESS, ERRORED);

      // Set deploy status to be returned
      deployStatus = DEPLOY_ERROR;
      SetDeployError("Deployment failed. The forked off process returned error code.");
      sLog(log_error, "Deployment failed. "
                      "The forked off process returned error code. \n");
   } else {
      // Set vmx status - customization success
      SetCustomizationStatusInVmx(TOOLSDEPLOYPKG_DONE,
                                  TOOLSDEPLOYPKG_ERROR_SUCCESS,
                                  NULL);

      // Move to DONE state
      TransitionState(INPROGRESS, DONE);

      deployStatus = DEPLOY_SUCCESS;
      sLog(log_info, "Deployment succeded. \n");
   }

   /*
    * Read in nics to enable from the nics.txt file. We do it irrespective of the
    * sucess/failure of the customization so that at the end of it we always
    * have nics enabled.
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

   // Clean up command
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

   //Reset the guest OS
   if (!sSkipReboot && !deploymentResult) {
      // Reboot the Vm
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