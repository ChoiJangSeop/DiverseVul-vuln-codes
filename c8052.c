S3BootScriptSaveSmbusExecute (
  IN  UINTN                             SmBusAddress,
  IN  EFI_SMBUS_OPERATION               Operation,
  IN  UINTN                             *Length,
  IN  VOID                              *Buffer
  )
{
  EFI_STATUS            Status;
  UINTN                 BufferLength;
  UINT8                 DataSize;
  UINT8                *Script;
  EFI_BOOT_SCRIPT_SMBUS_EXECUTE  ScriptSmbusExecute;

  if (Length == NULL) {
    BufferLength = 0;
  } else {
    BufferLength = *Length;
  }

  Status = CheckParameters (SmBusAddress, Operation, &BufferLength, Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DataSize = (UINT8)(sizeof (EFI_BOOT_SCRIPT_SMBUS_EXECUTE) + BufferLength);

  Script = S3BootScriptGetEntryAddAddress (DataSize);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // Build script data
  //
  ScriptSmbusExecute.OpCode       = EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE;
  ScriptSmbusExecute.Length       = DataSize;
  ScriptSmbusExecute.SmBusAddress = (UINT64) SmBusAddress;
  ScriptSmbusExecute.Operation    = Operation;
  ScriptSmbusExecute.DataSize     = (UINT32) BufferLength;

  CopyMem ((VOID*)Script, (VOID*)&ScriptSmbusExecute, sizeof (EFI_BOOT_SCRIPT_SMBUS_EXECUTE));
  CopyMem (
    (VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_SMBUS_EXECUTE)),
    Buffer,
    BufferLength
    );

  SyncBootScript (Script);

  return RETURN_SUCCESS;
}