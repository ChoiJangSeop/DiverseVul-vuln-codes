S3BootScriptSavePciCfgWrite (
  IN  S3_BOOT_SCRIPT_LIB_WIDTH         Width,
  IN  UINT64                           Address,
  IN  UINTN                            Count,
  IN  VOID                            *Buffer
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte;
  EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE  ScriptPciWrite;

  if (Width == S3BootScriptWidthUint64 ||
      Width == S3BootScriptWidthFifoUint64 ||
      Width == S3BootScriptWidthFillUint64) {
    return EFI_INVALID_PARAMETER;
  }

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE) + (WidthInByte * Count));

  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // Build script data
  //
  ScriptPciWrite.OpCode   = EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE;
  ScriptPciWrite.Length   = Length;
  ScriptPciWrite.Width    = Width;
  ScriptPciWrite.Address  = Address;
  ScriptPciWrite.Count    = (UINT32) Count;

  CopyMem ((VOID*)Script, (VOID*)&ScriptPciWrite,  sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE)), Buffer, WidthInByte * Count);

  SyncBootScript (Script);

  return RETURN_SUCCESS;
}