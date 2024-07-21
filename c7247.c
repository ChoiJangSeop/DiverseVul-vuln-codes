InternalFindFile (
  IN   EFI_BLOCK_IO_PROTOCOL           *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL            *DiskIo,
  IN   UDF_VOLUME_INFO                 *Volume,
  IN   CHAR16                          *FileName,
  IN   UDF_FILE_INFO                   *Parent,
  IN   UDF_LONG_ALLOCATION_DESCRIPTOR  *Icb,
  OUT  UDF_FILE_INFO                   *File
  )
{
  EFI_STATUS                      Status;
  UDF_FILE_IDENTIFIER_DESCRIPTOR  *FileIdentifierDesc;
  UDF_READ_DIRECTORY_INFO         ReadDirInfo;
  BOOLEAN                         Found;
  CHAR16                          FoundFileName[UDF_FILENAME_LENGTH];
  VOID                            *CompareFileEntry;

  //
  // Check if both Parent->FileIdentifierDesc and Icb are NULL.
  //
  if ((Parent->FileIdentifierDesc == NULL) && (Icb == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if parent file is really directory.
  //
  if (FE_ICB_FILE_TYPE (Parent->FileEntry) != UdfFileEntryDirectory) {
    return EFI_NOT_FOUND;
  }

  //
  // If FileName is current file or working directory, just duplicate Parent's
  // FE/EFE and FID descriptors.
  //
  if (StrCmp (FileName, L".") == 0) {
    if (Parent->FileIdentifierDesc == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    DuplicateFe (BlockIo, Volume, Parent->FileEntry, &File->FileEntry);
    if (File->FileEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    DuplicateFid (Parent->FileIdentifierDesc, &File->FileIdentifierDesc);
    if (File->FileIdentifierDesc == NULL) {
      FreePool (File->FileEntry);
      return EFI_OUT_OF_RESOURCES;
    }

    return EFI_SUCCESS;
  }

  //
  // Start directory listing.
  //
  ZeroMem ((VOID *)&ReadDirInfo, sizeof (UDF_READ_DIRECTORY_INFO));
  Found = FALSE;

  for (;;) {
    Status = ReadDirectoryEntry (
      BlockIo,
      DiskIo,
      Volume,
      (Parent->FileIdentifierDesc != NULL) ?
      &Parent->FileIdentifierDesc->Icb :
      Icb,
      Parent->FileEntry,
      &ReadDirInfo,
      &FileIdentifierDesc
      );
    if (EFI_ERROR (Status)) {
      if (Status == EFI_DEVICE_ERROR) {
        Status = EFI_NOT_FOUND;
      }

      break;
    }
    //
    // After calling function ReadDirectoryEntry(), if 'FileIdentifierDesc' is
    // NULL, then the 'Status' must be EFI_OUT_OF_RESOURCES. Hence, if the code
    // reaches here, 'FileIdentifierDesc' must be not NULL.
    //
    // The ASSERT here is for addressing a false positive NULL pointer
    // dereference issue raised from static analysis.
    //
    ASSERT (FileIdentifierDesc != NULL);

    if (FileIdentifierDesc->FileCharacteristics & PARENT_FILE) {
      //
      // This FID contains the location (FE/EFE) of the parent directory of this
      // directory (Parent), and if FileName is either ".." or "\\", then it's
      // the expected FID.
      //
      if (StrCmp (FileName, L"..") == 0 || StrCmp (FileName, L"\\") == 0) {
        Found = TRUE;
        break;
      }
    } else {
      Status = GetFileNameFromFid (FileIdentifierDesc, FoundFileName);
      if (EFI_ERROR (Status)) {
        break;
      }

      if (StrCmp (FileName, FoundFileName) == 0) {
        //
        // FID has been found. Prepare to find its respective FE/EFE.
        //
        Found = TRUE;
        break;
      }
    }

    FreePool ((VOID *)FileIdentifierDesc);
  }

  if (ReadDirInfo.DirectoryData != NULL) {
    //
    // Free all allocated resources for the directory listing.
    //
    FreePool (ReadDirInfo.DirectoryData);
  }

  if (Found) {
    Status = EFI_SUCCESS;

    File->FileIdentifierDesc = FileIdentifierDesc;

    //
    // If the requested file is root directory, then the FE/EFE was already
    // retrieved in UdfOpenVolume() function, thus no need to find it again.
    //
    // Otherwise, find FE/EFE from the respective FID.
    //
    if (StrCmp (FileName, L"\\") != 0) {
      Status = FindFileEntry (
        BlockIo,
        DiskIo,
        Volume,
        &FileIdentifierDesc->Icb,
        &CompareFileEntry
        );
      if (EFI_ERROR (Status)) {
        goto Error_Find_Fe;
      }

      //
      // Make sure that both Parent's FE/EFE and found FE/EFE are not equal.
      //
      if (CompareMem ((VOID *)Parent->FileEntry, (VOID *)CompareFileEntry,
                      Volume->FileEntrySize) != 0) {
        File->FileEntry = CompareFileEntry;
      } else {
        FreePool ((VOID *)FileIdentifierDesc);
        FreePool ((VOID *)CompareFileEntry);
        Status = EFI_NOT_FOUND;
      }
    }
  }

  return Status;

Error_Find_Fe:
  FreePool ((VOID *)FileIdentifierDesc);

  return Status;
}