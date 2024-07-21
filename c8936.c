SetStringWorker (
  IN  HII_DATABASE_PRIVATE_DATA       *Private,
  IN OUT HII_STRING_PACKAGE_INSTANCE  *StringPackage,
  IN  EFI_STRING_ID                   StringId,
  IN  EFI_STRING                      String,
  IN  EFI_FONT_INFO                   *StringFontInfo OPTIONAL
  )
{
  UINT8                                *StringTextPtr;
  UINT8                                BlockType;
  UINT8                                *StringBlockAddr;
  UINTN                                StringTextOffset;
  EFI_STATUS                           Status;
  UINT8                                *Block;
  UINT8                                *BlockPtr;
  UINTN                                BlockSize;
  UINTN                                OldBlockSize;
  HII_FONT_INFO                        *LocalFont;
  HII_GLOBAL_FONT_INFO                 *GlobalFont;
  BOOLEAN                              Referred;
  EFI_HII_SIBT_EXT2_BLOCK              Ext2;
  UINTN                                StringSize;
  UINTN                                TmpSize;
  EFI_STRING_ID                        StartStringId;

  StartStringId = 0;
  StringSize    = 0;
  ASSERT (Private != NULL && StringPackage != NULL && String != NULL);
  ASSERT (Private->Signature == HII_DATABASE_PRIVATE_DATA_SIGNATURE);
  //
  // Find the specified string block
  //
  Status = FindStringBlock (
             Private,
             StringPackage,
             StringId,
             &BlockType,
             &StringBlockAddr,
             &StringTextOffset,
             NULL,
             &StartStringId
             );
  if (EFI_ERROR (Status) && (BlockType == EFI_HII_SIBT_SKIP1 || BlockType == EFI_HII_SIBT_SKIP2)) {
    Status = InsertLackStringBlock(StringPackage,
                          StartStringId,
                          StringId,
                          &BlockType,
                          &StringBlockAddr,
                          (BOOLEAN)(StringFontInfo != NULL)
                          );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    if (StringFontInfo != NULL) {
      StringTextOffset = sizeof (EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK) - sizeof (CHAR16);
    } else {
      StringTextOffset = sizeof (EFI_HII_SIBT_STRING_UCS2_BLOCK) - sizeof (CHAR16);
    }
  }

  LocalFont  = NULL;
  GlobalFont = NULL;
  Referred   = FALSE;

  //
  // The input StringFontInfo should exist in current database if specified.
  //
  if (StringFontInfo != NULL) {
    if (!IsFontInfoExisted (Private, StringFontInfo, NULL, NULL, &GlobalFont)) {
      return EFI_INVALID_PARAMETER;
    } else {
      Referred = ReferFontInfoLocally (
                   Private,
                   StringPackage,
                   StringPackage->FontId,
                   FALSE,
                   GlobalFont,
                   &LocalFont
                   );
      if (!Referred) {
        StringPackage->FontId++;
      }
    }
    //
    // Update the FontId of the specified string block to input font info.
    //
    switch (BlockType) {
    case EFI_HII_SIBT_STRING_SCSU_FONT:
    case EFI_HII_SIBT_STRINGS_SCSU_FONT:
    case EFI_HII_SIBT_STRING_UCS2_FONT:
    case EFI_HII_SIBT_STRINGS_UCS2_FONT:
      *(StringBlockAddr + sizeof (EFI_HII_STRING_BLOCK)) = LocalFont->FontId;
      break;
    default:
      //
      // When modify the font info of these blocks, the block type should be updated
      // to contain font info thus the whole structure should be revised.
      // It is recommended to use tool to modify the block type not in the code.
      //
      return EFI_UNSUPPORTED;
    }
  }

  OldBlockSize = StringPackage->StringPkgHdr->Header.Length - StringPackage->StringPkgHdr->HdrSize;

  //
  // Set the string text and font.
  //
  StringTextPtr = StringBlockAddr + StringTextOffset;
  switch (BlockType) {
  case EFI_HII_SIBT_STRING_SCSU:
  case EFI_HII_SIBT_STRING_SCSU_FONT:
  case EFI_HII_SIBT_STRINGS_SCSU:
  case EFI_HII_SIBT_STRINGS_SCSU_FONT:
    BlockSize = OldBlockSize + StrLen (String);
    BlockSize -= AsciiStrSize ((CHAR8 *) StringTextPtr);
    Block = AllocateZeroPool (BlockSize);
    if (Block == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Block, StringPackage->StringBlock, StringTextPtr - StringPackage->StringBlock);
    BlockPtr = Block + (StringTextPtr - StringPackage->StringBlock);

    while (*String != 0) {
      *BlockPtr++ = (CHAR8) *String++;
    }
    *BlockPtr++ = 0;


    TmpSize = OldBlockSize - (StringTextPtr - StringPackage->StringBlock) - AsciiStrSize ((CHAR8 *) StringTextPtr);
    CopyMem (
      BlockPtr,
      StringTextPtr + AsciiStrSize ((CHAR8 *)StringTextPtr),
      TmpSize
      );

    FreePool (StringPackage->StringBlock);
    StringPackage->StringBlock = Block;
    StringPackage->StringPkgHdr->Header.Length += (UINT32) (BlockSize - OldBlockSize);
    break;

  case EFI_HII_SIBT_STRING_UCS2:
  case EFI_HII_SIBT_STRING_UCS2_FONT:
  case EFI_HII_SIBT_STRINGS_UCS2:
  case EFI_HII_SIBT_STRINGS_UCS2_FONT:
    //
    // Use StrSize to store the size of the specified string, including the NULL
    // terminator.
    //
    GetUnicodeStringTextOrSize (NULL, StringTextPtr, &StringSize);

    BlockSize = OldBlockSize + StrSize (String) - StringSize;
    Block = AllocateZeroPool (BlockSize);
    if (Block == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Block, StringPackage->StringBlock, StringTextPtr - StringPackage->StringBlock);
    BlockPtr = Block + (StringTextPtr - StringPackage->StringBlock);

    CopyMem (BlockPtr, String, StrSize (String));
    BlockPtr += StrSize (String);

    CopyMem (
      BlockPtr,
      StringTextPtr + StringSize,
      OldBlockSize - (StringTextPtr - StringPackage->StringBlock) - StringSize
      );

    FreePool (StringPackage->StringBlock);
    StringPackage->StringBlock = Block;
    StringPackage->StringPkgHdr->Header.Length += (UINT32) (BlockSize - OldBlockSize);
    break;

  default:
    return EFI_NOT_FOUND;
  }

  //
  // Insert a new EFI_HII_SIBT_FONT_BLOCK to the header of string block, if incoming
  // StringFontInfo does not exist in current string package.
  //
  // This new block does not impact on the value of StringId.
  //
  //
  if (StringFontInfo == NULL || Referred) {
    return EFI_SUCCESS;
  }

  OldBlockSize = StringPackage->StringPkgHdr->Header.Length - StringPackage->StringPkgHdr->HdrSize;
  BlockSize = OldBlockSize + sizeof (EFI_HII_SIBT_FONT_BLOCK) - sizeof (CHAR16) +
              StrSize (GlobalFont->FontInfo->FontName);

  Block = AllocateZeroPool (BlockSize);
  if (Block == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BlockPtr = Block;
  Ext2.Header.BlockType = EFI_HII_SIBT_EXT2;
  Ext2.BlockType2       = EFI_HII_SIBT_FONT;
  Ext2.Length           = (UINT16) (BlockSize - OldBlockSize);
  CopyMem (BlockPtr, &Ext2, sizeof (EFI_HII_SIBT_EXT2_BLOCK));
  BlockPtr += sizeof (EFI_HII_SIBT_EXT2_BLOCK);

  *BlockPtr = LocalFont->FontId;
  BlockPtr ++;
  CopyMem (BlockPtr, &GlobalFont->FontInfo->FontSize, sizeof (UINT16));
  BlockPtr += sizeof (UINT16);
  CopyMem (BlockPtr, &GlobalFont->FontInfo->FontStyle, sizeof (UINT32));
  BlockPtr += sizeof (UINT32);
  CopyMem (
    BlockPtr,
    GlobalFont->FontInfo->FontName,
    StrSize (GlobalFont->FontInfo->FontName)
    );
  BlockPtr += StrSize (GlobalFont->FontInfo->FontName);

  CopyMem (BlockPtr, StringPackage->StringBlock, OldBlockSize);

  FreePool (StringPackage->StringBlock);
  StringPackage->StringBlock = Block;
  StringPackage->StringPkgHdr->Header.Length += Ext2.Length;

  return EFI_SUCCESS;

}