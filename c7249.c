UdfOpen (
  IN   EFI_FILE_PROTOCOL  *This,
  OUT  EFI_FILE_PROTOCOL  **NewHandle,
  IN   CHAR16             *FileName,
  IN   UINT64             OpenMode,
  IN   UINT64             Attributes
  )
{
  EFI_TPL                     OldTpl;
  EFI_STATUS                  Status;
  PRIVATE_UDF_FILE_DATA       *PrivFileData;
  PRIVATE_UDF_SIMPLE_FS_DATA  *PrivFsData;
  CHAR16                      FilePath[UDF_PATH_LENGTH];
  UDF_FILE_INFO               File;
  PRIVATE_UDF_FILE_DATA       *NewPrivFileData;
  CHAR16                      *TempFileName;

  ZeroMem (FilePath, sizeof FilePath);
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (This == NULL || NewHandle == NULL || FileName == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Error_Invalid_Params;
  }

  if (OpenMode != EFI_FILE_MODE_READ) {
    Status = EFI_WRITE_PROTECTED;
    goto Error_Invalid_Params;
  }

  PrivFileData = PRIVATE_UDF_FILE_DATA_FROM_THIS (This);

  PrivFsData = PRIVATE_UDF_SIMPLE_FS_DATA_FROM_THIS (PrivFileData->SimpleFs);

  //
  // Build full path
  //
  if (*FileName == L'\\') {
    StrCpyS (FilePath, UDF_PATH_LENGTH, FileName);
  } else {
    StrCpyS (FilePath, UDF_PATH_LENGTH, PrivFileData->AbsoluteFileName);
    StrCatS (FilePath, UDF_PATH_LENGTH, L"\\");
    StrCatS (FilePath, UDF_PATH_LENGTH, FileName);
  }

  MangleFileName (FilePath);
  if (FilePath[0] == L'\0') {
    Status = EFI_NOT_FOUND;
    goto Error_Bad_FileName;
  }

  Status = FindFile (
    PrivFsData->BlockIo,
    PrivFsData->DiskIo,
    &PrivFsData->Volume,
    FilePath,
    _ROOT_FILE (PrivFileData),
    _PARENT_FILE (PrivFileData),
    &_PARENT_FILE(PrivFileData)->FileIdentifierDesc->Icb,
    &File
    );
  if (EFI_ERROR (Status)) {
    goto Error_Find_File;
  }

  NewPrivFileData =
    (PRIVATE_UDF_FILE_DATA *)AllocateZeroPool (sizeof (PRIVATE_UDF_FILE_DATA));
  if (NewPrivFileData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error_Alloc_New_Priv_File_Data;
  }

  CopyMem ((VOID *)NewPrivFileData, (VOID *)PrivFileData,
           sizeof (PRIVATE_UDF_FILE_DATA));
  CopyMem ((VOID *)&NewPrivFileData->File, &File, sizeof (UDF_FILE_INFO));

  NewPrivFileData->IsRootDirectory = FALSE;

  StrCpyS (NewPrivFileData->AbsoluteFileName, UDF_PATH_LENGTH, FilePath);
  FileName = NewPrivFileData->AbsoluteFileName;

  while ((TempFileName = StrStr (FileName, L"\\")) != NULL) {
    FileName = TempFileName + 1;
  }

  StrCpyS (NewPrivFileData->FileName, UDF_PATH_LENGTH, FileName);

  Status = GetFileSize (
    PrivFsData->BlockIo,
    PrivFsData->DiskIo,
    &PrivFsData->Volume,
    &NewPrivFileData->File,
    &NewPrivFileData->FileSize
    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: GetFileSize() fails with status - %r.\n",
      __FUNCTION__, Status
      ));
    goto Error_Get_File_Size;
  }

  NewPrivFileData->FilePosition = 0;
  ZeroMem ((VOID *)&NewPrivFileData->ReadDirInfo,
           sizeof (UDF_READ_DIRECTORY_INFO));

  *NewHandle = &NewPrivFileData->FileIo;

  PrivFsData->OpenFiles++;

  gBS->RestoreTPL (OldTpl);

  return Status;

Error_Get_File_Size:
  FreePool ((VOID *)NewPrivFileData);

Error_Alloc_New_Priv_File_Data:
  CleanupFileInformation (&File);

Error_Find_File:
Error_Bad_FileName:
Error_Invalid_Params:
  gBS->RestoreTPL (OldTpl);

  return Status;
}