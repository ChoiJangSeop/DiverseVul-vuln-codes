S3BootScriptSaveInformation (
  IN  UINT32                                InformationLength,
  IN  VOID                                 *Information
  )
{
  UINT8                 Length;
  UINT8                 *Script;
  EFI_BOOT_SCRIPT_INFORMATION  ScriptInformation;

  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_INFORMATION) + InformationLength);

  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // Build script data
  //
  ScriptInformation.OpCode     = EFI_BOOT_SCRIPT_INFORMATION_OPCODE;
  ScriptInformation.Length     = Length;


  ScriptInformation.InformationLength = InformationLength;

  CopyMem ((VOID*)Script, (VOID*)&ScriptInformation, sizeof (EFI_BOOT_SCRIPT_INFORMATION));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_INFORMATION)), (VOID *) Information, (UINTN) InformationLength);

  SyncBootScript (Script);

  return RETURN_SUCCESS;

}