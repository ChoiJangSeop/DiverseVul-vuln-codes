FindFile (
  IN   EFI_BLOCK_IO_PROTOCOL           *BlockIo,
  IN   EFI_DISK_IO_PROTOCOL            *DiskIo,
  IN   UDF_VOLUME_INFO                 *Volume,
  IN   CHAR16                          *FilePath,
  IN   UDF_FILE_INFO                   *Root,
  IN   UDF_FILE_INFO                   *Parent,
  IN   UDF_LONG_ALLOCATION_DESCRIPTOR  *Icb,
  OUT  UDF_FILE_INFO                   *File
  )
{
  EFI_STATUS     Status;
  CHAR16         FileName[UDF_FILENAME_LENGTH];
  CHAR16         *FileNamePointer;
  UDF_FILE_INFO  PreviousFile;
  VOID           *FileEntry;

  Status = EFI_NOT_FOUND;

  CopyMem ((VOID *)&PreviousFile, (VOID *)Parent, sizeof (UDF_FILE_INFO));
  while (*FilePath != L'\0') {
    FileNamePointer = FileName;
    while (*FilePath != L'\0' && *FilePath != L'\\') {
      *FileNamePointer++ = *FilePath++;
    }

    *FileNamePointer = L'\0';
    if (FileName[0] == L'\0') {
      //
      // Open root directory.
      //
      if (Root == NULL) {
        //
        // There is no file found for the root directory yet. So, find only its
        // FID by now.
        //
        // See UdfOpenVolume() function.
        //
        Status = InternalFindFile (BlockIo,
                                   DiskIo,
                                   Volume,
                                   L"\\",
                                   &PreviousFile,
                                   Icb,
                                   File);
      } else {
        //
        // We've already a file pointer (Root) for the root directory. Duplicate
        // its FE/EFE and FID descriptors.
        //
        Status = EFI_SUCCESS;
        DuplicateFe (BlockIo, Volume, Root->FileEntry, &File->FileEntry);
        if (File->FileEntry == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          //
          // File->FileEntry is not NULL.
          //
          DuplicateFid (Root->FileIdentifierDesc, &File->FileIdentifierDesc);
          if (File->FileIdentifierDesc == NULL) {
            FreePool (File->FileEntry);
            Status = EFI_OUT_OF_RESOURCES;
          }
        }
      }
    } else {
      //
      // No root directory. Find filename from the current directory.
      //
      Status = InternalFindFile (BlockIo,
                                 DiskIo,
                                 Volume,
                                 FileName,
                                 &PreviousFile,
                                 Icb,
                                 File);
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // If the found file is a symlink, then find its respective FE/EFE and
    // FID descriptors.
    //
    if (FE_ICB_FILE_TYPE (File->FileEntry) == UdfFileEntrySymlink) {
      FreePool ((VOID *)File->FileIdentifierDesc);

      FileEntry = File->FileEntry;

      Status = ResolveSymlink (BlockIo,
                               DiskIo,
                               Volume,
                               &PreviousFile,
                               FileEntry,
                               File);

      FreePool (FileEntry);

      if (EFI_ERROR (Status)) {
        return Status;
      }
    }

    if (CompareMem ((VOID *)&PreviousFile, (VOID *)Parent,
                    sizeof (UDF_FILE_INFO)) != 0) {
      CleanupFileInformation (&PreviousFile);
    }

    CopyMem ((VOID *)&PreviousFile, (VOID *)File, sizeof (UDF_FILE_INFO));
    if (*FilePath != L'\0' && *FilePath == L'\\') {
      FilePath++;
    }
  }

  return Status;
}