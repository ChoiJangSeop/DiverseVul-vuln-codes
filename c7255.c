ResolveSymlink (
  IN   EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL   *DiskIo,
  IN   UDF_VOLUME_INFO        *Volume,
  IN   UDF_FILE_INFO          *Parent,
  IN   VOID                   *FileEntryData,
  OUT  UDF_FILE_INFO          *File
  )
{
  EFI_STATUS          Status;
  UDF_READ_FILE_INFO  ReadFileInfo;
  UINT8               *Data;
  UINT64              Length;
  UINT8               *EndData;
  UDF_PATH_COMPONENT  *PathComp;
  UINT8               PathCompLength;
  CHAR16              FileName[UDF_FILENAME_LENGTH];
  CHAR16              *Char;
  UINTN               Index;
  UINT8               CompressionId;
  UDF_FILE_INFO       PreviousFile;

  //
  // Symlink files on UDF volumes do not contain so much data other than
  // Path Components which resolves to real filenames, so it's OK to read in
  // all its data here -- usually the data will be inline with the FE/EFE for
  // lower filenames.
  //
  ReadFileInfo.Flags = ReadFileAllocateAndRead;

  Status = ReadFile (
    BlockIo,
    DiskIo,
    Volume,
    &Parent->FileIdentifierDesc->Icb,
    FileEntryData,
    &ReadFileInfo
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Length = ReadFileInfo.ReadLength;

  Data = (UINT8 *)ReadFileInfo.FileData;
  EndData = Data + Length;

  CopyMem ((VOID *)&PreviousFile, (VOID *)Parent, sizeof (UDF_FILE_INFO));

  for (;;) {
    PathComp = (UDF_PATH_COMPONENT *)Data;

    PathCompLength = PathComp->LengthOfComponentIdentifier;

    switch (PathComp->ComponentType) {
    case 1:
      //
      // This Path Component specifies the root directory hierarchy subject to
      // agreement between the originator and recipient of the medium. Skip it.
      //
      // Fall through.
      //
    case 2:
      //
      // "\\." of the current directory. Read next Path Component.
      //
      goto Next_Path_Component;
    case 3:
      //
      // ".." (parent directory). Go to it.
      //
      CopyMem ((VOID *)FileName, L"..", 6);
      break;
    case 4:
      //
      // "." (current file). Duplicate both FE/EFE and FID of this file.
      //
      DuplicateFe (BlockIo, Volume, PreviousFile.FileEntry, &File->FileEntry);
      if (File->FileEntry == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Error_Find_File;
      }

      DuplicateFid (PreviousFile.FileIdentifierDesc,
                    &File->FileIdentifierDesc);
      if (File->FileIdentifierDesc == NULL) {
        FreePool (File->FileEntry);
        Status = EFI_OUT_OF_RESOURCES;
        goto Error_Find_File;
      }
      goto Next_Path_Component;
    case 5:
      //
      // This Path Component identifies an object, either a file or a
      // directory or an alias.
      //
      // Decode it from the compressed data in ComponentIdentifier and find
      // respective path.
      //
      CompressionId = PathComp->ComponentIdentifier[0];
      if (!IS_VALID_COMPRESSION_ID (CompressionId)) {
        return EFI_VOLUME_CORRUPTED;
      }

      Char = FileName;
      for (Index = 1; Index < PathCompLength; Index++) {
        if (CompressionId == 16) {
          *Char = *(UINT8 *)((UINT8 *)PathComp->ComponentIdentifier +
                          Index) << 8;
          Index++;
        } else {
          if (Index > ARRAY_SIZE (FileName)) {
            return EFI_UNSUPPORTED;
          }
          *Char = 0;
        }

        if (Index < Length) {
          *Char |= (CHAR16)(*(UINT8 *)((UINT8 *)PathComp->ComponentIdentifier + Index));
        }

        Char++;
      }

      Index = ((UINTN)Char - (UINTN)FileName) / sizeof (CHAR16);
      if (Index > ARRAY_SIZE (FileName) - 1) {
        Index = ARRAY_SIZE (FileName) - 1;
      }
      FileName[Index] = L'\0';
      break;
    }

    //
    // Find file from the read filename in symlink's file data.
    //
    Status = InternalFindFile (
      BlockIo,
      DiskIo,
      Volume,
      FileName,
      &PreviousFile,
      NULL,
      File
      );
    if (EFI_ERROR (Status)) {
      goto Error_Find_File;
    }

  Next_Path_Component:
    Data += sizeof (UDF_PATH_COMPONENT) + PathCompLength;
    if (Data >= EndData) {
      break;
    }

    if (CompareMem ((VOID *)&PreviousFile, (VOID *)Parent,
                    sizeof (UDF_FILE_INFO)) != 0) {
      CleanupFileInformation (&PreviousFile);
    }

    CopyMem ((VOID *)&PreviousFile, (VOID *)File, sizeof (UDF_FILE_INFO));
  }

  //
  // Unmap the symlink file.
  //
  FreePool (ReadFileInfo.FileData);

  return EFI_SUCCESS;

Error_Find_File:
  if (CompareMem ((VOID *)&PreviousFile, (VOID *)Parent,
                  sizeof (UDF_FILE_INFO)) != 0) {
    CleanupFileInformation (&PreviousFile);
  }

  FreePool (ReadFileInfo.FileData);

  return Status;
}