S3BootScriptSaveMemWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH          Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  VOID                              *Buffer
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte;
  EFI_BOOT_SCRIPT_MEM_WRITE  ScriptMemWrite;

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_MEM_WRITE) + (WidthInByte * Count));

  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // Build script data
  //
  ScriptMemWrite.OpCode   = EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE;
  ScriptMemWrite.Length   = Length;
  ScriptMemWrite.Width    = Width;
  ScriptMemWrite.Address  = Address;
  ScriptMemWrite.Count    = (UINT32) Count;

  CopyMem ((VOID*)Script, (VOID*)&ScriptMemWrite, sizeof(EFI_BOOT_SCRIPT_MEM_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_MEM_WRITE)), Buffer, WidthInByte * Count);

  SyncBootScript (Script);

  return RETURN_SUCCESS;
}