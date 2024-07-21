UdfRead (
  IN      EFI_FILE_PROTOCOL  *This,
  IN OUT  UINTN              *BufferSize,
  OUT     VOID               *Buffer
  )
{
  EFI_TPL                         OldTpl;
  EFI_STATUS                      Status;
  PRIVATE_UDF_FILE_DATA           *PrivFileData;
  PRIVATE_UDF_SIMPLE_FS_DATA      *PrivFsData;
  UDF_VOLUME_INFO                 *Volume;
  UDF_FILE_INFO                   *Parent;
  UDF_READ_DIRECTORY_INFO         *ReadDirInfo;
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  EFI_DISK_IO_PROTOCOL            *DiskIo;
  UDF_FILE_INFO                   FoundFile;
  UDF_FILE_IDENTIFIER_DESCRIPTOR  *NewFileIdentifierDesc;
  VOID                            *NewFileEntryData;
  CHAR16                          FileName[UDF_FILENAME_LENGTH];
  UINT64                          FileSize;
  UINT64                          BufferSizeUint64;

  ZeroMem (FileName, sizeof FileName);
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (This == NULL || BufferSize == NULL || (*BufferSize != 0 &&
                                             Buffer == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Error_Invalid_Params;
  }

  PrivFileData = PRIVATE_UDF_FILE_DATA_FROM_THIS (This);
  PrivFsData = PRIVATE_UDF_SIMPLE_FS_DATA_FROM_THIS (PrivFileData->SimpleFs);

  BlockIo                = PrivFsData->BlockIo;
  DiskIo                 = PrivFsData->DiskIo;
  Volume                 = &PrivFsData->Volume;
  ReadDirInfo            = &PrivFileData->ReadDirInfo;
  NewFileIdentifierDesc  = NULL;
  NewFileEntryData       = NULL;

  Parent = _PARENT_FILE (PrivFileData);

  Status = EFI_VOLUME_CORRUPTED;

  if (IS_FID_NORMAL_FILE (Parent->FileIdentifierDesc)) {
    if (PrivFileData->FilePosition > PrivFileData->FileSize) {
      //
      // File's position is beyond the EOF
      //
      Status = EFI_DEVICE_ERROR;
      goto Error_File_Beyond_The_Eof;
    }

    if (PrivFileData->FilePosition == PrivFileData->FileSize) {
      *BufferSize = 0;
      Status = EFI_SUCCESS;
      goto Done;
    }

    BufferSizeUint64 = *BufferSize;

    Status = ReadFileData (
      BlockIo,
      DiskIo,
      Volume,
      Parent,
      PrivFileData->FileSize,
      &PrivFileData->FilePosition,
      Buffer,
      &BufferSizeUint64
      );
    ASSERT (BufferSizeUint64 <= MAX_UINTN);
    *BufferSize = (UINTN)BufferSizeUint64;
  } else if (IS_FID_DIRECTORY_FILE (Parent->FileIdentifierDesc)) {
    if (ReadDirInfo->FidOffset == 0 && PrivFileData->FilePosition > 0) {
      Status = EFI_DEVICE_ERROR;
      *BufferSize = 0;
      goto Done;
    }

    for (;;) {
      Status = ReadDirectoryEntry (
        BlockIo,
        DiskIo,
        Volume,
        &Parent->FileIdentifierDesc->Icb,
        Parent->FileEntry,
        ReadDirInfo,
        &NewFileIdentifierDesc
        );
      if (EFI_ERROR (Status)) {
        if (Status == EFI_DEVICE_ERROR) {
          FreePool (ReadDirInfo->DirectoryData);
          ZeroMem ((VOID *)ReadDirInfo, sizeof (UDF_READ_DIRECTORY_INFO));

          *BufferSize = 0;
          Status = EFI_SUCCESS;
        }

        goto Done;
      }
      //
      // After calling function ReadDirectoryEntry(), if 'NewFileIdentifierDesc'
      // is NULL, then the 'Status' must be EFI_OUT_OF_RESOURCES. Hence, if the
      // code reaches here, 'NewFileIdentifierDesc' must be not NULL.
      //
      // The ASSERT here is for addressing a false positive NULL pointer
      // dereference issue raised from static analysis.
      //
      ASSERT (NewFileIdentifierDesc != NULL);

      if (!IS_FID_PARENT_FILE (NewFileIdentifierDesc)) {
        break;
      }

      FreePool ((VOID *)NewFileIdentifierDesc);
    }

    Status = FindFileEntry (
      BlockIo,
      DiskIo,
      Volume,
      &NewFileIdentifierDesc->Icb,
      &NewFileEntryData
      );
    if (EFI_ERROR (Status)) {
      goto Error_Find_Fe;
    }
    ASSERT (NewFileEntryData != NULL);

    if (FE_ICB_FILE_TYPE (NewFileEntryData) == UdfFileEntrySymlink) {
      Status = ResolveSymlink (
        BlockIo,
        DiskIo,
        Volume,
        Parent,
        NewFileEntryData,
        &FoundFile
        );
      if (EFI_ERROR (Status)) {
        goto Error_Resolve_Symlink;
      }

      FreePool ((VOID *)NewFileEntryData);
      NewFileEntryData = FoundFile.FileEntry;

      Status = GetFileNameFromFid (NewFileIdentifierDesc, FileName);
      if (EFI_ERROR (Status)) {
        FreePool ((VOID *)FoundFile.FileIdentifierDesc);
        goto Error_Get_FileName;
      }

      FreePool ((VOID *)NewFileIdentifierDesc);
      NewFileIdentifierDesc = FoundFile.FileIdentifierDesc;
    } else {
      FoundFile.FileIdentifierDesc  = NewFileIdentifierDesc;
      FoundFile.FileEntry           = NewFileEntryData;

      Status = GetFileNameFromFid (FoundFile.FileIdentifierDesc, FileName);
      if (EFI_ERROR (Status)) {
        goto Error_Get_FileName;
      }
    }

    Status = GetFileSize (
      BlockIo,
      DiskIo,
      Volume,
      &FoundFile,
      &FileSize
      );
    if (EFI_ERROR (Status)) {
      goto Error_Get_File_Size;
    }

    Status = SetFileInfo (
      &FoundFile,
      FileSize,
      FileName,
      BufferSize,
      Buffer
      );
    if (EFI_ERROR (Status)) {
      goto Error_Set_File_Info;
    }

    PrivFileData->FilePosition++;
    Status = EFI_SUCCESS;
  } else if (IS_FID_DELETED_FILE (Parent->FileIdentifierDesc)) {
    //
    // Code should never reach here.
    //
    ASSERT (FALSE);
    Status = EFI_DEVICE_ERROR;
  }

Error_Set_File_Info:
Error_Get_File_Size:
Error_Get_FileName:
Error_Resolve_Symlink:
  if (NewFileEntryData != NULL) {
    FreePool (NewFileEntryData);
  }

Error_Find_Fe:
  if (NewFileIdentifierDesc != NULL) {
    FreePool ((VOID *)NewFileIdentifierDesc);
  }

Done:
Error_File_Beyond_The_Eof:
Error_Invalid_Params:
  gBS->RestoreTPL (OldTpl);

  return Status;
}