UdfGetInfo (
  IN      EFI_FILE_PROTOCOL  *This,
  IN      EFI_GUID           *InformationType,
  IN OUT  UINTN              *BufferSize,
  OUT     VOID               *Buffer
  )
{
  EFI_STATUS                  Status;
  PRIVATE_UDF_FILE_DATA       *PrivFileData;
  PRIVATE_UDF_SIMPLE_FS_DATA  *PrivFsData;
  EFI_FILE_SYSTEM_INFO        *FileSystemInfo;
  UINTN                       FileSystemInfoLength;
  CHAR16                      *String;
  UDF_FILE_SET_DESCRIPTOR     *FileSetDesc;
  UINTN                       Index;
  UINT8                       *OstaCompressed;
  UINT8                       CompressionId;
  UINT64                      VolumeSize;
  UINT64                      FreeSpaceSize;
  CHAR16                      VolumeLabel[64];

  if (This == NULL || InformationType == NULL || BufferSize == NULL ||
      (*BufferSize != 0 && Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivFileData = PRIVATE_UDF_FILE_DATA_FROM_THIS (This);

  PrivFsData = PRIVATE_UDF_SIMPLE_FS_DATA_FROM_THIS (PrivFileData->SimpleFs);

  Status = EFI_UNSUPPORTED;

  if (CompareGuid (InformationType, &gEfiFileInfoGuid)) {
    Status = SetFileInfo (
      _FILE (PrivFileData),
      PrivFileData->FileSize,
      PrivFileData->FileName,
      BufferSize,
      Buffer
      );
  } else if (CompareGuid (InformationType, &gEfiFileSystemInfoGuid)) {
    String = VolumeLabel;

    FileSetDesc = &PrivFsData->Volume.FileSetDesc;

    OstaCompressed = &FileSetDesc->LogicalVolumeIdentifier[0];

    CompressionId = OstaCompressed[0];
    if (!IS_VALID_COMPRESSION_ID (CompressionId)) {
      return EFI_VOLUME_CORRUPTED;
    }

    for (Index = 1; Index < 128; Index++) {
      if (CompressionId == 16) {
        *String = *(UINT8 *)(OstaCompressed + Index) << 8;
        Index++;
      } else {
        *String = 0;
      }

      if (Index < 128) {
        *String |= (CHAR16)(*(UINT8 *)(OstaCompressed + Index));
      }

      //
      // Unlike FID Identifiers, Logical Volume Identifier is stored in a
      // NULL-terminated OSTA compressed format, so we must check for the NULL
      // character.
      //
      if (*String == L'\0') {
        break;
      }

      String++;
    }

    *String = L'\0';

    FileSystemInfoLength = StrSize (VolumeLabel) +
                           sizeof (EFI_FILE_SYSTEM_INFO);
    if (*BufferSize < FileSystemInfoLength) {
      *BufferSize = FileSystemInfoLength;
      return EFI_BUFFER_TOO_SMALL;
    }

    FileSystemInfo = (EFI_FILE_SYSTEM_INFO *)Buffer;
    StrCpyS (FileSystemInfo->VolumeLabel, ARRAY_SIZE (VolumeLabel),
             VolumeLabel);
    Status = GetVolumeSize (
      PrivFsData->BlockIo,
      PrivFsData->DiskIo,
      &PrivFsData->Volume,
      &VolumeSize,
      &FreeSpaceSize
      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    FileSystemInfo->Size        = FileSystemInfoLength;
    FileSystemInfo->ReadOnly    = TRUE;
    FileSystemInfo->BlockSize   =
      PrivFsData->Volume.LogicalVolDesc.LogicalBlockSize;
    FileSystemInfo->VolumeSize  = VolumeSize;
    FileSystemInfo->FreeSpace   = FreeSpaceSize;

    *BufferSize = FileSystemInfoLength;
    Status = EFI_SUCCESS;
  }

  return Status;
}