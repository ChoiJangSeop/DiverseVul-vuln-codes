HiiGetDatabaseInfo(
  IN CONST EFI_HII_DATABASE_PROTOCOL        *This
  )
{
  EFI_STATUS                          Status;
  EFI_HII_PACKAGE_LIST_HEADER         *DatabaseInfo;
  UINTN                               DatabaseInfoSize;

  DatabaseInfo         = NULL;
  DatabaseInfoSize     = 0;

  //
  // Get HiiDatabase information.
  //
  Status = HiiExportPackageLists(This, NULL, &DatabaseInfoSize, DatabaseInfo);

  ASSERT(Status == EFI_BUFFER_TOO_SMALL);

  if(DatabaseInfoSize > gDatabaseInfoSize ) {
    //
    // Do 25% overallocation to minimize the number of memory allocations after ReadyToBoot.
    // Since lots of allocation after ReadyToBoot may change memory map and cause S4 resume issue.
    //
    gDatabaseInfoSize = DatabaseInfoSize + (DatabaseInfoSize >> 2);
    if (gRTDatabaseInfoBuffer != NULL){
      FreePool(gRTDatabaseInfoBuffer);
      DEBUG ((DEBUG_WARN, "[HiiDatabase]: Memory allocation is required after ReadyToBoot, which may change memory map and cause S4 resume issue.\n"));
    }
    gRTDatabaseInfoBuffer = AllocateRuntimeZeroPool (gDatabaseInfoSize);
    if (gRTDatabaseInfoBuffer == NULL){
      DEBUG ((DEBUG_ERROR, "[HiiDatabase]: No enough memory resource to store the HiiDatabase info.\n"));
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    ZeroMem(gRTDatabaseInfoBuffer,gDatabaseInfoSize);
  }
  Status = HiiExportPackageLists(This, NULL, &DatabaseInfoSize, gRTDatabaseInfoBuffer);
  ASSERT_EFI_ERROR (Status);
  gBS->InstallConfigurationTable (&gEfiHiiDatabaseProtocolGuid, gRTDatabaseInfoBuffer);

  return EFI_SUCCESS;

}