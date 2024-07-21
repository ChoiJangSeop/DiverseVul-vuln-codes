HiiNewImage (
  IN  CONST EFI_HII_IMAGE_PROTOCOL   *This,
  IN  EFI_HII_HANDLE                 PackageList,
  OUT EFI_IMAGE_ID                   *ImageId,
  IN  CONST EFI_IMAGE_INPUT          *Image
  )
{
  HII_DATABASE_PRIVATE_DATA           *Private;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_IMAGE_PACKAGE_INSTANCE          *ImagePackage;
  EFI_HII_IMAGE_BLOCK                 *ImageBlocks;
  UINT32                              NewBlockSize;

  if (This == NULL || ImageId == NULL || Image == NULL || Image->Bitmap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = HII_IMAGE_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  PackageListNode = LocatePackageList (&Private->DatabaseList, PackageList);
  if (PackageListNode == NULL) {
    return EFI_NOT_FOUND;
  }

  EfiAcquireLock (&mHiiDatabaseLock);

  NewBlockSize = sizeof (EFI_HII_IIBT_IMAGE_24BIT_BLOCK) - sizeof (EFI_HII_RGB_PIXEL) +
                 BITMAP_LEN_24_BIT ((UINT32) Image->Width, Image->Height);

  //
  // Get the image package in the package list,
  // or create a new image package if image package does not exist.
  //
  if (PackageListNode->ImagePkg != NULL) {
    ImagePackage = PackageListNode->ImagePkg;

    //
    // Output the image id of the incoming image being inserted, which is the
    // image id of the EFI_HII_IIBT_END block of old image package.
    //
    *ImageId = 0;
    GetImageIdOrAddress (ImagePackage->ImageBlock, ImageId);

    //
    // Update the package's image block by appending the new block to the end.
    //
    ImageBlocks = AllocatePool (ImagePackage->ImageBlockSize + NewBlockSize);
    if (ImageBlocks == NULL) {
      EfiReleaseLock (&mHiiDatabaseLock);
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Copy the original content.
    //
    CopyMem (
      ImageBlocks,
      ImagePackage->ImageBlock,
      ImagePackage->ImageBlockSize - sizeof (EFI_HII_IIBT_END_BLOCK)
      );
    FreePool (ImagePackage->ImageBlock);
    ImagePackage->ImageBlock = ImageBlocks;

    //
    // Point to the very last block.
    //
    ImageBlocks = (EFI_HII_IMAGE_BLOCK *) (
                    (UINT8 *) ImageBlocks + ImagePackage->ImageBlockSize - sizeof (EFI_HII_IIBT_END_BLOCK)
                    );
    //
    // Update the length record.
    //
    ImagePackage->ImageBlockSize                  += NewBlockSize;
    ImagePackage->ImagePkgHdr.Header.Length       += NewBlockSize;
    PackageListNode->PackageListHdr.PackageLength += NewBlockSize;

  } else {
    //
    // The specified package list does not contain image package.
    // Create one to add this image block.
    //
    ImagePackage = (HII_IMAGE_PACKAGE_INSTANCE *) AllocateZeroPool (sizeof (HII_IMAGE_PACKAGE_INSTANCE));
    if (ImagePackage == NULL) {
      EfiReleaseLock (&mHiiDatabaseLock);
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Output the image id of the incoming image being inserted, which is the
    // first image block so that id is initially to one.
    //
    *ImageId = 1;
    //
    // Fill in image package header.
    //
    ImagePackage->ImagePkgHdr.Header.Length     = sizeof (EFI_HII_IMAGE_PACKAGE_HDR) + NewBlockSize + sizeof (EFI_HII_IIBT_END_BLOCK);
    ImagePackage->ImagePkgHdr.Header.Type       = EFI_HII_PACKAGE_IMAGES;
    ImagePackage->ImagePkgHdr.ImageInfoOffset   = sizeof (EFI_HII_IMAGE_PACKAGE_HDR);
    ImagePackage->ImagePkgHdr.PaletteInfoOffset = 0;

    //
    // Fill in palette info.
    //
    ImagePackage->PaletteBlock    = NULL;
    ImagePackage->PaletteInfoSize = 0;

    //
    // Fill in image blocks.
    //
    ImagePackage->ImageBlockSize = NewBlockSize + sizeof (EFI_HII_IIBT_END_BLOCK);
    ImagePackage->ImageBlock = AllocateZeroPool (NewBlockSize + sizeof (EFI_HII_IIBT_END_BLOCK));
    if (ImagePackage->ImageBlock == NULL) {
      FreePool (ImagePackage);
      EfiReleaseLock (&mHiiDatabaseLock);
      return EFI_OUT_OF_RESOURCES;
    }
    ImageBlocks = ImagePackage->ImageBlock;

    //
    // Insert this image package.
    //
    PackageListNode->ImagePkg = ImagePackage;
    PackageListNode->PackageListHdr.PackageLength += ImagePackage->ImagePkgHdr.Header.Length;
  }

  //
  // Append the new block here
  //
  if (Image->Flags == EFI_IMAGE_TRANSPARENT) {
    ImageBlocks->BlockType = EFI_HII_IIBT_IMAGE_24BIT_TRANS;
  } else {
    ImageBlocks->BlockType = EFI_HII_IIBT_IMAGE_24BIT;
  }
  WriteUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) ImageBlocks)->Bitmap.Width, Image->Width);
  WriteUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) ImageBlocks)->Bitmap.Height, Image->Height);
  CopyGopToRgbPixel (((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) ImageBlocks)->Bitmap.Bitmap, Image->Bitmap, (UINT32) Image->Width * Image->Height);

  //
  // Append the block end
  //
  ImageBlocks = (EFI_HII_IMAGE_BLOCK *) ((UINT8 *) ImageBlocks + NewBlockSize);
  ImageBlocks->BlockType = EFI_HII_IIBT_END;

  //
  // Check whether need to get the contents of HiiDataBase.
  // Only after ReadyToBoot to do the export.
  //
  if (gExportAfterReadyToBoot) {
    HiiGetDatabaseInfo(&Private->HiiDatabase);
  }

  EfiReleaseLock (&mHiiDatabaseLock);

  return EFI_SUCCESS;
}