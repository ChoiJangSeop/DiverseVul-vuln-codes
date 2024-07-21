HiiNewString (
  IN  CONST EFI_HII_STRING_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST CHAR8                     *Language,
  IN  CONST CHAR16                    *LanguageName, OPTIONAL
  IN  CONST EFI_STRING                String,
  IN  CONST EFI_FONT_INFO             *StringFontInfo OPTIONAL
  )
{
  EFI_STATUS                          Status;
  LIST_ENTRY                          *Link;
  HII_DATABASE_PRIVATE_DATA           *Private;
  HII_DATABASE_RECORD                 *DatabaseRecord;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_STRING_PACKAGE_INSTANCE         *StringPackage;
  UINT32                              HeaderSize;
  UINT32                              BlockSize;
  UINT32                              OldBlockSize;
  UINT8                               *StringBlock;
  UINT8                               *BlockPtr;
  UINT32                              Ucs2BlockSize;
  UINT32                              FontBlockSize;
  UINT32                              Ucs2FontBlockSize;
  EFI_HII_SIBT_EXT2_BLOCK             Ext2;
  HII_FONT_INFO                       *LocalFont;
  HII_GLOBAL_FONT_INFO                *GlobalFont;
  EFI_STRING_ID                       NewStringId;
  EFI_STRING_ID                       NextStringId;
  EFI_STRING_ID                       Index;
  HII_STRING_PACKAGE_INSTANCE         *MatchStringPackage;
  BOOLEAN                             NewStringPackageCreated;


  if (This == NULL || String == NULL || StringId == NULL || Language == NULL || PackageList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsHiiHandleValid (PackageList)) {
    return EFI_NOT_FOUND;
  }

  Private    = HII_STRING_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  GlobalFont = NULL;

  //
  // If StringFontInfo specify a paritcular font, it should exist in current database.
  //
  if (StringFontInfo != NULL) {
    if (!IsFontInfoExisted (Private, (EFI_FONT_INFO *) StringFontInfo, NULL, NULL, &GlobalFont)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Get the matching package list.
  //
  PackageListNode = NULL;
  for (Link = Private->DatabaseList.ForwardLink; Link != &Private->DatabaseList; Link = Link->ForwardLink) {
    DatabaseRecord = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
    if (DatabaseRecord->Handle == PackageList) {
      PackageListNode = DatabaseRecord->PackageList;
      break;
    }
  }
  if (PackageListNode == NULL) {
    return EFI_NOT_FOUND;
  }

  EfiAcquireLock (&mHiiDatabaseLock);

  Status = EFI_SUCCESS;
  NewStringPackageCreated = FALSE;
  NewStringId   = 0;
  NextStringId  = 0;
  StringPackage = NULL;
  MatchStringPackage = NULL;
  for (Link = PackageListNode->StringPkgHdr.ForwardLink;
       Link != &PackageListNode->StringPkgHdr;
       Link = Link->ForwardLink
      ) {
    StringPackage = CR (Link, HII_STRING_PACKAGE_INSTANCE, StringEntry, HII_STRING_PACKAGE_SIGNATURE);
    //
    // Create a string block and corresponding font block if exists, then append them
    // to the end of the string package.
    //
    Status = FindStringBlock (
               Private,
               StringPackage,
               0,
               NULL,
               NULL,
               NULL,
               &NextStringId,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    //
    // Make sure that new StringId is same in all String Packages for the different language.
    //
    if (NewStringId != 0 && NewStringId != NextStringId) {
      ASSERT (FALSE);
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }
    NewStringId = NextStringId;
    //
    // Get the matched string package with language.
    //
    if (HiiCompareLanguage (StringPackage->StringPkgHdr->Language, (CHAR8 *) Language)) {
      MatchStringPackage = StringPackage;
    } else {
      OldBlockSize = StringPackage->StringPkgHdr->Header.Length - StringPackage->StringPkgHdr->HdrSize;
      //
      // Create a blank EFI_HII_SIBT_STRING_UCS2_BLOCK to reserve new string ID.
      //
      Ucs2BlockSize = (UINT32) sizeof (EFI_HII_SIBT_STRING_UCS2_BLOCK);

      StringBlock = (UINT8 *) AllocateZeroPool (OldBlockSize + Ucs2BlockSize);
      if (StringBlock == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      //
      // Copy original string blocks, except the EFI_HII_SIBT_END.
      //
      CopyMem (StringBlock, StringPackage->StringBlock, OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK));
      //
      // Create a blank EFI_HII_SIBT_STRING_UCS2 block
      //
      BlockPtr  = StringBlock + OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK);
      *BlockPtr = EFI_HII_SIBT_STRING_UCS2;
      BlockPtr  += sizeof (EFI_HII_SIBT_STRING_UCS2_BLOCK);

      //
      // Append a EFI_HII_SIBT_END block to the end.
      //
      *BlockPtr = EFI_HII_SIBT_END;
      FreePool (StringPackage->StringBlock);
      StringPackage->StringBlock = StringBlock;
      StringPackage->StringPkgHdr->Header.Length += Ucs2BlockSize;
      PackageListNode->PackageListHdr.PackageLength += Ucs2BlockSize;
    }
  }
  if (NewStringId == 0) {
    //
    // No string package is found.
    // Create new string package. StringId 1 is reserved for Language Name string.
    //
    *StringId = 2;
  } else {
    //
    // Set new StringId
    //
    *StringId = (EFI_STRING_ID) (NewStringId + 1);
  }

  if (MatchStringPackage != NULL) {
    StringPackage = MatchStringPackage;
  } else {
    //
    // LanguageName is required to create a new string package.
    //
    if (LanguageName == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    StringPackage = AllocateZeroPool (sizeof (HII_STRING_PACKAGE_INSTANCE));
    if (StringPackage == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    StringPackage->Signature   = HII_STRING_PACKAGE_SIGNATURE;
    StringPackage->MaxStringId = *StringId;
    StringPackage->FontId      = 0;
    InitializeListHead (&StringPackage->FontInfoList);

    //
    // Fill in the string package header
    //
    HeaderSize = (UINT32) (AsciiStrSize ((CHAR8 *) Language) - 1 + sizeof (EFI_HII_STRING_PACKAGE_HDR));
    StringPackage->StringPkgHdr = AllocateZeroPool (HeaderSize);
    if (StringPackage->StringPkgHdr == NULL) {
      FreePool (StringPackage);
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }
    StringPackage->StringPkgHdr->Header.Type      = EFI_HII_PACKAGE_STRINGS;
    StringPackage->StringPkgHdr->HdrSize          = HeaderSize;
    StringPackage->StringPkgHdr->StringInfoOffset = HeaderSize;
    CopyMem (StringPackage->StringPkgHdr->LanguageWindow, mLanguageWindow, 16 * sizeof (CHAR16));
    StringPackage->StringPkgHdr->LanguageName     = 1;
    AsciiStrCpyS (StringPackage->StringPkgHdr->Language, (HeaderSize - OFFSET_OF(EFI_HII_STRING_PACKAGE_HDR,Language)) / sizeof (CHAR8), (CHAR8 *) Language);

    //
    // Calculate the length of the string blocks, including string block to record
    // printable language full name and EFI_HII_SIBT_END_BLOCK.
    //
    Ucs2BlockSize = (UINT32) (StrSize ((CHAR16 *) LanguageName) +
                              (*StringId - 1) * sizeof (EFI_HII_SIBT_STRING_UCS2_BLOCK) - sizeof (CHAR16));

    BlockSize     = Ucs2BlockSize + sizeof (EFI_HII_SIBT_END_BLOCK);
    StringPackage->StringBlock = (UINT8 *) AllocateZeroPool (BlockSize);
    if (StringPackage->StringBlock == NULL) {
      FreePool (StringPackage->StringPkgHdr);
      FreePool (StringPackage);
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    //
    // Insert the string block of printable language full name
    //
    BlockPtr  = StringPackage->StringBlock;
    *BlockPtr = EFI_HII_SIBT_STRING_UCS2;
    BlockPtr  += sizeof (EFI_HII_STRING_BLOCK);
    CopyMem (BlockPtr, (EFI_STRING) LanguageName, StrSize ((EFI_STRING) LanguageName));
    BlockPtr += StrSize ((EFI_STRING) LanguageName);
    for (Index = 2; Index <= *StringId - 1; Index ++) {
      *BlockPtr = EFI_HII_SIBT_STRING_UCS2;
      BlockPtr += sizeof (EFI_HII_SIBT_STRING_UCS2_BLOCK);
    }
    //
    // Insert the end block
    //
    *BlockPtr = EFI_HII_SIBT_END;

    //
    // Append this string package node to string package array in this package list.
    //
    StringPackage->StringPkgHdr->Header.Length    = HeaderSize + BlockSize;
    PackageListNode->PackageListHdr.PackageLength += StringPackage->StringPkgHdr->Header.Length;
    InsertTailList (&PackageListNode->StringPkgHdr, &StringPackage->StringEntry);
    NewStringPackageCreated = TRUE;
  }

  OldBlockSize = StringPackage->StringPkgHdr->Header.Length - StringPackage->StringPkgHdr->HdrSize;

  if (StringFontInfo == NULL) {
    //
    // Create a EFI_HII_SIBT_STRING_UCS2_BLOCK since font info is not specified.
    //
    Ucs2BlockSize = (UINT32) (StrSize (String) + sizeof (EFI_HII_SIBT_STRING_UCS2_BLOCK)
                              - sizeof (CHAR16));

    StringBlock = (UINT8 *) AllocateZeroPool (OldBlockSize + Ucs2BlockSize);
    if (StringBlock == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }
    //
    // Copy original string blocks, except the EFI_HII_SIBT_END.
    //
    CopyMem (StringBlock, StringPackage->StringBlock, OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK));
    //
    // Create a EFI_HII_SIBT_STRING_UCS2 block
    //
    BlockPtr  = StringBlock + OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK);
    *BlockPtr = EFI_HII_SIBT_STRING_UCS2;
    BlockPtr  += sizeof (EFI_HII_STRING_BLOCK);
    CopyMem (BlockPtr, (EFI_STRING) String, StrSize ((EFI_STRING) String));
    BlockPtr += StrSize ((EFI_STRING) String);

    //
    // Append a EFI_HII_SIBT_END block to the end.
    //
    *BlockPtr = EFI_HII_SIBT_END;
    FreePool (StringPackage->StringBlock);
    StringPackage->StringBlock = StringBlock;
    StringPackage->StringPkgHdr->Header.Length += Ucs2BlockSize;
    PackageListNode->PackageListHdr.PackageLength += Ucs2BlockSize;

  } else {
    //
    // StringFontInfo is specified here. If there is a EFI_HII_SIBT_FONT_BLOCK
    // which refers to this font info, create a EFI_HII_SIBT_STRING_UCS2_FONT block
    // only. Otherwise create a EFI_HII_SIBT_FONT block with a EFI_HII_SIBT_STRING
    // _UCS2_FONT block.
    //
    Ucs2FontBlockSize = (UINT32) (StrSize (String) + sizeof (EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK) -
                                  sizeof (CHAR16));
    if (ReferFontInfoLocally (Private, StringPackage, StringPackage->FontId, FALSE, GlobalFont, &LocalFont)) {
      //
      // Create a EFI_HII_SIBT_STRING_UCS2_FONT block only.
      //
      StringBlock = (UINT8 *) AllocateZeroPool (OldBlockSize + Ucs2FontBlockSize);
      if (StringBlock == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      //
      // Copy original string blocks, except the EFI_HII_SIBT_END.
      //
      CopyMem (StringBlock, StringPackage->StringBlock, OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK));
      //
      // Create a EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK
      //
      BlockPtr  = StringBlock + OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK);
      *BlockPtr = EFI_HII_SIBT_STRING_UCS2_FONT;
      BlockPtr  += sizeof (EFI_HII_STRING_BLOCK);
      *BlockPtr = LocalFont->FontId;
      BlockPtr ++;
      CopyMem (BlockPtr, (EFI_STRING) String, StrSize ((EFI_STRING) String));
      BlockPtr += StrSize ((EFI_STRING) String);

      //
      // Append a EFI_HII_SIBT_END block to the end.
      //
      *BlockPtr = EFI_HII_SIBT_END;
      FreePool (StringPackage->StringBlock);
      StringPackage->StringBlock = StringBlock;
      StringPackage->StringPkgHdr->Header.Length += Ucs2FontBlockSize;
      PackageListNode->PackageListHdr.PackageLength += Ucs2FontBlockSize;

    } else {
      //
      // EFI_HII_SIBT_FONT_BLOCK does not exist in current string package, so
      // create a EFI_HII_SIBT_FONT block to record the font info, then generate
      // a EFI_HII_SIBT_STRING_UCS2_FONT block to record the incoming string.
      //
      FontBlockSize = (UINT32) (StrSize (((EFI_FONT_INFO *) StringFontInfo)->FontName) +
                                sizeof (EFI_HII_SIBT_FONT_BLOCK) - sizeof (CHAR16));
      StringBlock = (UINT8 *) AllocateZeroPool (OldBlockSize + FontBlockSize + Ucs2FontBlockSize);
      if (StringBlock == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      //
      // Copy original string blocks, except the EFI_HII_SIBT_END.
      //
      CopyMem (StringBlock, StringPackage->StringBlock, OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK));

      //
      // Create a EFI_HII_SIBT_FONT block firstly and then backup its info in string
      // package instance for future reference.
      //
      BlockPtr = StringBlock + OldBlockSize - sizeof (EFI_HII_SIBT_END_BLOCK);

      Ext2.Header.BlockType = EFI_HII_SIBT_EXT2;
      Ext2.BlockType2       = EFI_HII_SIBT_FONT;
      Ext2.Length           = (UINT16) FontBlockSize;
      CopyMem (BlockPtr, &Ext2, sizeof (EFI_HII_SIBT_EXT2_BLOCK));
      BlockPtr += sizeof (EFI_HII_SIBT_EXT2_BLOCK);

      *BlockPtr = LocalFont->FontId;
      BlockPtr ++;
      CopyMem (BlockPtr, &((EFI_FONT_INFO *) StringFontInfo)->FontSize, sizeof (UINT16));
      BlockPtr += sizeof (UINT16);
      CopyMem (BlockPtr, &((EFI_FONT_INFO *) StringFontInfo)->FontStyle, sizeof (EFI_HII_FONT_STYLE));
      BlockPtr += sizeof (EFI_HII_FONT_STYLE);
      CopyMem (
        BlockPtr,
        &((EFI_FONT_INFO *) StringFontInfo)->FontName,
        StrSize (((EFI_FONT_INFO *) StringFontInfo)->FontName)
        );
      BlockPtr += StrSize (((EFI_FONT_INFO *) StringFontInfo)->FontName);
      //
      // Create a EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK
      //
      *BlockPtr = EFI_HII_SIBT_STRING_UCS2_FONT;
      BlockPtr  += sizeof (EFI_HII_STRING_BLOCK);
      *BlockPtr = LocalFont->FontId;
      BlockPtr  ++;
      CopyMem (BlockPtr, (EFI_STRING) String, StrSize ((EFI_STRING) String));
      BlockPtr += StrSize ((EFI_STRING) String);

      //
      // Append a EFI_HII_SIBT_END block to the end.
      //
      *BlockPtr = EFI_HII_SIBT_END;
      FreePool (StringPackage->StringBlock);
      StringPackage->StringBlock = StringBlock;
      StringPackage->StringPkgHdr->Header.Length += FontBlockSize + Ucs2FontBlockSize;
      PackageListNode->PackageListHdr.PackageLength += FontBlockSize + Ucs2FontBlockSize;

      //
      // Increase the FontId to make it unique since we already add
      // a EFI_HII_SIBT_FONT block to this string package.
      //
      StringPackage->FontId++;
    }
  }

Done:
  if (!EFI_ERROR (Status) && NewStringPackageCreated) {
    //
    // Trigger any registered notification function for new string package
    //
    Status = InvokeRegisteredFunction (
      Private,
      EFI_HII_DATABASE_NOTIFY_NEW_PACK,
      (VOID *) StringPackage,
      EFI_HII_PACKAGE_STRINGS,
      PackageList
      );
  }

  if (!EFI_ERROR (Status)) {
    //
    // Update MaxString Id to new StringId
    //
    for (Link = PackageListNode->StringPkgHdr.ForwardLink;
      Link != &PackageListNode->StringPkgHdr;
      Link = Link->ForwardLink
      ) {
        StringPackage = CR (Link, HII_STRING_PACKAGE_INSTANCE, StringEntry, HII_STRING_PACKAGE_SIGNATURE);
        StringPackage->MaxStringId = *StringId;
    }
  } else if (NewStringPackageCreated) {
    //
    // Free the allocated new string Package when new string can't be added.
    //
    RemoveEntryList (&StringPackage->StringEntry);
    FreePool (StringPackage->StringBlock);
    FreePool (StringPackage->StringPkgHdr);
    FreePool (StringPackage);
  }
  //
  // The contents of HiiDataBase may updated,need to check.
  //
  //
  // Check whether need to get the contents of HiiDataBase.
  // Only after ReadyToBoot to do the export.
  //
  if (gExportAfterReadyToBoot) {
    if (!EFI_ERROR (Status)) {
      HiiGetDatabaseInfo(&Private->HiiDatabase);
    }
  }

  EfiReleaseLock (&mHiiDatabaseLock);

  return Status;
}