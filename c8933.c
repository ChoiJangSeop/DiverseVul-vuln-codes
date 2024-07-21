HiiGetConfigRespInfo(
  IN CONST EFI_HII_DATABASE_PROTOCOL        *This
  )
{
  EFI_STATUS                          Status;
  HII_DATABASE_PRIVATE_DATA           *Private;
  EFI_STRING                          ConfigAltResp;
  UINTN                               ConfigSize;

  ConfigAltResp        = NULL;
  ConfigSize           = 0;

  Private = HII_DATABASE_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  //
  // Get ConfigResp string
  //
  Status = HiiConfigRoutingExportConfig(&Private->ConfigRouting,&ConfigAltResp);

  if (!EFI_ERROR (Status)){
    ConfigSize = StrSize(ConfigAltResp);
    if (ConfigSize > gConfigRespSize){
      //
      // Do 25% overallocation to minimize the number of memory allocations after ReadyToBoot.
      // Since lots of allocation after ReadyToBoot may change memory map and cause S4 resume issue.
      //
      gConfigRespSize = ConfigSize + (ConfigSize >> 2);
      if (gRTConfigRespBuffer != NULL){
        FreePool(gRTConfigRespBuffer);
        DEBUG ((DEBUG_WARN, "[HiiDatabase]: Memory allocation is required after ReadyToBoot, which may change memory map and cause S4 resume issue.\n"));
      }
      gRTConfigRespBuffer = (EFI_STRING) AllocateRuntimeZeroPool (gConfigRespSize);
      if (gRTConfigRespBuffer == NULL){
        FreePool(ConfigAltResp);
        DEBUG ((DEBUG_ERROR, "[HiiDatabase]: No enough memory resource to store the ConfigResp string.\n"));
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      ZeroMem(gRTConfigRespBuffer,gConfigRespSize);
    }
    CopyMem(gRTConfigRespBuffer,ConfigAltResp,ConfigSize);
    gBS->InstallConfigurationTable (&gEfiHiiConfigRoutingProtocolGuid, gRTConfigRespBuffer);
    FreePool(ConfigAltResp);
  }

  return EFI_SUCCESS;

}