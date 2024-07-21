GetFileNameFromFid (
  IN   UDF_FILE_IDENTIFIER_DESCRIPTOR  *FileIdentifierDesc,
  OUT  CHAR16                          *FileName
  )
{
  UINT8 *OstaCompressed;
  UINT8 CompressionId;
  UINT8 Length;
  UINTN Index;

  OstaCompressed =
    (UINT8 *)(
      (UINT8 *)FileIdentifierDesc->Data +
      FileIdentifierDesc->LengthOfImplementationUse
      );

  CompressionId = OstaCompressed[0];
  if (!IS_VALID_COMPRESSION_ID (CompressionId)) {
    return EFI_VOLUME_CORRUPTED;
  }

  //
  // Decode filename.
  //
  Length = FileIdentifierDesc->LengthOfFileIdentifier;
  for (Index = 1; Index < Length; Index++) {
    if (CompressionId == 16) {
      *FileName = OstaCompressed[Index++] << 8;
    } else {
      *FileName = 0;
    }

    if (Index < Length) {
      *FileName |= (CHAR16)(OstaCompressed[Index]);
    }

    FileName++;
  }

  *FileName = L'\0';

  return EFI_SUCCESS;
}