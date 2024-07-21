S3BootScriptLabelInternal (
  IN        BOOLEAN                        BeforeOrAfter,
  IN OUT    VOID                         **Position OPTIONAL,
  IN        UINT32                         InformationLength,
  IN CONST  CHAR8                          *Information
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
  ScriptInformation.OpCode     = S3_BOOT_SCRIPT_LIB_LABEL_OPCODE;
  ScriptInformation.Length     = Length;


  ScriptInformation.InformationLength = InformationLength;

  CopyMem ((VOID*)Script, (VOID*)&ScriptInformation, sizeof (EFI_BOOT_SCRIPT_INFORMATION));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_INFORMATION)), (VOID *) Information, (UINTN) InformationLength);

  SyncBootScript (Script);

  return S3BootScriptMoveLastOpcode (BeforeOrAfter, Position);

}