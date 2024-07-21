S3BootScriptSavePciCfg2Write (
  IN S3_BOOT_SCRIPT_LIB_WIDTH        Width,
  IN UINT16                          Segment,
  IN UINT64                          Address,
  IN UINTN                           Count,
  IN VOID                           *Buffer
  )
{
  UINT8                 Length;
  UINT8                *Script;
  UINT8                 WidthInByte;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE  ScriptPciWrite2;

  if (Width == S3BootScriptWidthUint64 ||
      Width == S3BootScriptWidthFifoUint64 ||
      Width == S3BootScriptWidthFillUint64) {
    return EFI_INVALID_PARAMETER;
  }

  WidthInByte = (UINT8) (0x01 << (Width & 0x03));
  Length = (UINT8)(sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE) + (WidthInByte * Count));

  Script = S3BootScriptGetEntryAddAddress (Length);
  if (Script == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }
  //
  // Build script data
  //
  ScriptPciWrite2.OpCode   = EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE;
  ScriptPciWrite2.Length   = Length;
  ScriptPciWrite2.Width    = Width;
  ScriptPciWrite2.Address  = Address;
  ScriptPciWrite2.Segment  = Segment;
  ScriptPciWrite2.Count    = (UINT32)Count;

  CopyMem ((VOID*)Script, (VOID*)&ScriptPciWrite2, sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE));
  CopyMem ((VOID*)(Script + sizeof (EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE)), Buffer, WidthInByte * Count);

  SyncBootScript (Script);

  return RETURN_SUCCESS;
}