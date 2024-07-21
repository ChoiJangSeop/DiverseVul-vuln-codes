S3BootScriptSaveIoWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH          Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  VOID                              *Buffer
  )

{
  UINT8                     Length;
  UINT8                    *Script;
  UINT8                     WidthInByte;
  EFI_BOOT_SCRIPT_IO_WRITE  ScriptIoWrite;

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_IO_WRITE) + (WidthInByte * Count));

  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // save script data
  //
  ScriptIoWrite.OpCode  = EFI_BOOT_SCRIPT_IO_WRITE_OPCODE;
  ScriptIoWrite.Length  = Length;
  ScriptIoWrite.Width   = Width;
  ScriptIoWrite.Address = Address;
  ScriptIoWrite.Count   = (UINT32) Count;
  CopyMem ((VOID*)Script, (VOID*)&ScriptIoWrite, sizeof(EFI_BOOT_SCRIPT_IO_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_IO_WRITE)), Buffer, WidthInByte * Count);

  SyncBootScript (Script);

  return RETURN_SUCCESS;
}