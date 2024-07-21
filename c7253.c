FindAnchorVolumeDescriptorPointer (
  IN   EFI_BLOCK_IO_PROTOCOL                 *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL                  *DiskIo,
  OUT  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  *AnchorPoint,
  OUT  EFI_LBA                               *LastRecordedBlock
  )
{
  EFI_STATUS                            Status;
  UINT32                                BlockSize;
  EFI_LBA                               EndLBA;
  UDF_DESCRIPTOR_TAG                    *DescriptorTag;
  UINTN                                 AvdpsCount;
  UINTN                                 Size;
  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  *AnchorPoints;
  INTN                                  Index;
  UDF_ANCHOR_VOLUME_DESCRIPTOR_POINTER  *AnchorPointPtr;
  EFI_LBA                               LastAvdpBlockNum;

  //
  // UDF 2.60, 2.2.3 Anchor Volume Descriptor Pointer
  //
  // An Anchor Volume Descriptor Pointer structure shall be recorded in at
  // least 2 of the following 3 locations on the media: Logical Sector 256,
  // N - 256 or N, where N is the last *addressable* sector of a volume.
  //
  // To figure out what logical sector N is, the SCSI commands READ CAPACITY and
  // READ TRACK INFORMATION are used, however many drives or medias report their
  // "last recorded block" wrongly. Although, READ CAPACITY returns the last
  // readable data block but there might be unwritten blocks, which are located
  // outside any track and therefore AVDP will not be found at block N.
  //
  // That said, we define a magic number of 512 blocks to be used as correction
  // when attempting to find AVDP and define last block number.
  //
  BlockSize = BlockIo->Media->BlockSize;
  EndLBA = BlockIo->Media->LastBlock;
  *LastRecordedBlock = EndLBA;
  AvdpsCount = 0;

  //
  // Find AVDP at block 256
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 (256, BlockSize),
    sizeof (*AnchorPoint),
    AnchorPoint
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DescriptorTag = &AnchorPoint->DescriptorTag;

  //
  // Check if read block is a valid AVDP descriptor
  //
  if (DescriptorTag->TagIdentifier == UdfAnchorVolumeDescriptorPointer) {
    DEBUG ((DEBUG_INFO, "%a: found AVDP at block %d\n", __FUNCTION__, 256));
    AvdpsCount++;
  }

  //
  // Find AVDP at block N - 256
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 ((UINT64)EndLBA - 256, BlockSize),
    sizeof (*AnchorPoint),
    AnchorPoint
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if read block is a valid AVDP descriptor
  //
  if (DescriptorTag->TagIdentifier == UdfAnchorVolumeDescriptorPointer &&
      ++AvdpsCount == 2) {
    DEBUG ((DEBUG_INFO, "%a: found AVDP at block %Ld\n", __FUNCTION__,
            EndLBA - 256));
    return EFI_SUCCESS;
  }

  //
  // Check if at least one AVDP was found in previous locations
  //
  if (AvdpsCount == 0) {
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // Find AVDP at block N
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 ((UINT64)EndLBA, BlockSize),
    sizeof (*AnchorPoint),
    AnchorPoint
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check if read block is a valid AVDP descriptor
  //
  if (DescriptorTag->TagIdentifier == UdfAnchorVolumeDescriptorPointer) {
    return EFI_SUCCESS;
  }

  //
  // No AVDP found at block N. Possibly drive/media returned bad last recorded
  // block, or it is part of unwritten data blocks and outside any track.
  //
  // Search backwards for an AVDP from block N-1 through
  // N-MAX_CORRECTION_BLOCKS_NUM. If any AVDP is found, then correct last block
  // number for the new UDF partition child handle.
  //
  Size = MAX_CORRECTION_BLOCKS_NUM * BlockSize;

  AnchorPoints = AllocateZeroPool (Size);
  if (AnchorPoints == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read consecutive MAX_CORRECTION_BLOCKS_NUM disk blocks
  //
  Status = DiskIo->ReadDisk (
    DiskIo,
    BlockIo->Media->MediaId,
    MultU64x32 ((UINT64)EndLBA - MAX_CORRECTION_BLOCKS_NUM, BlockSize),
    Size,
    AnchorPoints
    );
  if (EFI_ERROR (Status)) {
    goto Out_Free;
  }

  Status = EFI_VOLUME_CORRUPTED;

  //
  // Search for AVDP from blocks N-1 through N-MAX_CORRECTION_BLOCKS_NUM
  //
  for (Index = MAX_CORRECTION_BLOCKS_NUM - 2; Index >= 0; Index--) {
    AnchorPointPtr = (VOID *)((UINTN)AnchorPoints + Index * BlockSize);

    DescriptorTag = &AnchorPointPtr->DescriptorTag;

    //
    // Check if read block is a valid AVDP descriptor
    //
    if (DescriptorTag->TagIdentifier == UdfAnchorVolumeDescriptorPointer) {
      //
      // Calculate last recorded block number
      //
      LastAvdpBlockNum = EndLBA - (MAX_CORRECTION_BLOCKS_NUM - Index);
      DEBUG ((DEBUG_WARN, "%a: found AVDP at block %Ld\n", __FUNCTION__,
              LastAvdpBlockNum));
      DEBUG ((DEBUG_WARN, "%a: correcting last block from %Ld to %Ld\n",
              __FUNCTION__, EndLBA, LastAvdpBlockNum));
      //
      // Save read AVDP from last block
      //
      CopyMem (AnchorPoint, AnchorPointPtr, sizeof (*AnchorPointPtr));
      //
      // Set last recorded block number
      //
      *LastRecordedBlock = LastAvdpBlockNum;
      Status = EFI_SUCCESS;
      break;
    }
  }

Out_Free:
  FreePool (AnchorPoints);
  return Status;
}