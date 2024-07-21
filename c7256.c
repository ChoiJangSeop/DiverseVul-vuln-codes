GetVolumeSize (
  IN   EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL   *DiskIo,
  IN   UDF_VOLUME_INFO        *Volume,
  OUT  UINT64                 *VolumeSize,
  OUT  UINT64                 *FreeSpaceSize
  )
{
  EFI_STATUS                     Status;
  UDF_LOGICAL_VOLUME_DESCRIPTOR  *LogicalVolDesc;
  UDF_EXTENT_AD                  *ExtentAd;
  UINT64                         Lsn;
  UINT32                         LogicalBlockSize;
  UDF_LOGICAL_VOLUME_INTEGRITY   *LogicalVolInt;
  UDF_DESCRIPTOR_TAG             *DescriptorTag;
  UINTN                          Index;
  UINTN                          Length;
  UINT32                         LsnsNo;

  LogicalVolDesc = &Volume->LogicalVolDesc;

  ExtentAd = &LogicalVolDesc->IntegritySequenceExtent;

  if (ExtentAd->ExtentLength == 0) {
    return EFI_VOLUME_CORRUPTED;
  }

  LogicalVolInt = AllocatePool (ExtentAd->ExtentLength);
  if (LogicalVolInt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get location of Logical Volume Integrity Descriptor
  //
  Lsn = (UINT64)ExtentAd->ExtentLocation - Volume->MainVdsStartLocation;

  LogicalBlockSize = LogicalVolDesc->LogicalBlockSize;

  //
  // Read disk block
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 (Lsn, LogicalBlockSize),
    ExtentAd->ExtentLength,
    LogicalVolInt
    );
  if (EFI_ERROR (Status)) {
    goto Out_Free;
  }

  DescriptorTag = &LogicalVolInt->DescriptorTag;

  //
  // Check if read block is a Logical Volume Integrity Descriptor
  //
  if (DescriptorTag->TagIdentifier != UdfLogicalVolumeIntegrityDescriptor) {
    Status = EFI_VOLUME_CORRUPTED;
    goto Out_Free;
  }

  *VolumeSize = 0;
  *FreeSpaceSize = 0;

  Length = LogicalVolInt->NumberOfPartitions;
  for (Index = 0; Index < Length; Index += sizeof (UINT32)) {
    LsnsNo = *(UINT32 *)((UINT8 *)LogicalVolInt->Data + Index);
    //
    // Check if size is not specified
    //
    if (LsnsNo == 0xFFFFFFFFUL) {
      continue;
    }
    //
    // Accumulate free space size
    //
    *FreeSpaceSize += MultU64x32 ((UINT64)LsnsNo, LogicalBlockSize);
  }

  Length = LogicalVolInt->NumberOfPartitions * sizeof (UINT32) * 2;
  for (; Index < Length; Index += sizeof (UINT32)) {
    LsnsNo = *(UINT32 *)((UINT8 *)LogicalVolInt->Data + Index);
    //
    // Check if size is not specified
    //
    if (LsnsNo == 0xFFFFFFFFUL) {
      continue;
    }
    //
    // Accumulate used volume space
    //
    *VolumeSize += MultU64x32 ((UINT64)LsnsNo, LogicalBlockSize);
  }

  Status = EFI_SUCCESS;

Out_Free:
  //
  // Free Logical Volume Integrity Descriptor
  //
  FreePool (LogicalVolInt);

  return Status;
}