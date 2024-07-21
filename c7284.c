HiiSetImage (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_HANDLE                  PackageList,
  IN EFI_IMAGE_ID                    ImageId,
  IN CONST EFI_IMAGE_INPUT           *Image
  )
{
  HII_DATABASE_PRIVATE_DATA           *Private;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_IMAGE_PACKAGE_INSTANCE          *ImagePackage;
  EFI_HII_IMAGE_BLOCK                 *CurrentImageBlock;
  EFI_HII_IMAGE_BLOCK                 *ImageBlocks;
  EFI_HII_IMAGE_BLOCK                 *NewImageBlock;
  UINT32                              NewBlockSize;
  UINT32                              OldBlockSize;
  UINT32                               Part1Size;
  UINT32                               Part2Size;

  if (This == NULL || Image == NULL || ImageId == 0 || Image->Bitmap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = HII_IMAGE_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  PackageListNode = LocatePackageList (&Private->DatabaseList, PackageList);
  if (PackageListNode == NULL) {
    return EFI_NOT_FOUND;
  }
  ImagePackage = PackageListNode->ImagePkg;
  if (ImagePackage == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Find the image block specified by ImageId
  //
  CurrentImageBlock = GetImageIdOrAddress (ImagePackage->ImageBlock, &ImageId);
  if (CurrentImageBlock == NULL) {
    return EFI_NOT_FOUND;
  }

  EfiAcquireLock (&mHiiDatabaseLock);

  //
  // Get the size of original image block. Use some common block code here
  // since the definition of some structures is the same.
  //
  switch (CurrentImageBlock->BlockType) {
  case EFI_HII_IIBT_IMAGE_JPEG:
    OldBlockSize = OFFSET_OF (EFI_HII_IIBT_JPEG_BLOCK, Data) + ReadUnaligned32 ((VOID *) &((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Size);
    break;
  case EFI_HII_IIBT_IMAGE_PNG:
    OldBlockSize = OFFSET_OF (EFI_HII_IIBT_PNG_BLOCK, Data) + ReadUnaligned32 ((VOID *) &((EFI_HII_IIBT_PNG_BLOCK *) CurrentImageBlock)->Size);
    break;
  case EFI_HII_IIBT_IMAGE_1BIT:
  case EFI_HII_IIBT_IMAGE_1BIT_TRANS:
    OldBlockSize = sizeof (EFI_HII_IIBT_IMAGE_1BIT_BLOCK) - sizeof (UINT8) +
                   BITMAP_LEN_1_BIT (
                     ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_1BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                     ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_1BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                     );
    break;
  case EFI_HII_IIBT_IMAGE_4BIT:
  case EFI_HII_IIBT_IMAGE_4BIT_TRANS:
    OldBlockSize = sizeof (EFI_HII_IIBT_IMAGE_4BIT_BLOCK) - sizeof (UINT8) +
                   BITMAP_LEN_4_BIT (
                     ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_4BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                     ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_4BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                     );
    break;
  case EFI_HII_IIBT_IMAGE_8BIT:
  case EFI_HII_IIBT_IMAGE_8BIT_TRANS:
    OldBlockSize = sizeof (EFI_HII_IIBT_IMAGE_8BIT_BLOCK) - sizeof (UINT8) +
                   BITMAP_LEN_8_BIT (
                     (UINT32) ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_8BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                     ReadUnaligned16 (&((EFI_HII_IIBT_IMAGE_8BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                     );
    break;
  case EFI_HII_IIBT_IMAGE_24BIT:
  case EFI_HII_IIBT_IMAGE_24BIT_TRANS:
    OldBlockSize = sizeof (EFI_HII_IIBT_IMAGE_24BIT_BLOCK) - sizeof (EFI_HII_RGB_PIXEL) +
                   BITMAP_LEN_24_BIT (
                     (UINT32) ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width),
                     ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height)
                     );
    break;
  default:
    EfiReleaseLock (&mHiiDatabaseLock);
    return EFI_NOT_FOUND;
  }

  //
  // Create the new image block according to input image.
  //
  NewBlockSize = sizeof (EFI_HII_IIBT_IMAGE_24BIT_BLOCK) - sizeof (EFI_HII_RGB_PIXEL) +
                 BITMAP_LEN_24_BIT ((UINT32) Image->Width, Image->Height);
  //
  // Adjust the image package to remove the original block firstly then add the new block.
  //
  ImageBlocks = AllocateZeroPool (ImagePackage->ImageBlockSize + NewBlockSize - OldBlockSize);
  if (ImageBlocks == NULL) {
    EfiReleaseLock (&mHiiDatabaseLock);
    return EFI_OUT_OF_RESOURCES;
  }

  Part1Size = (UINT32) ((UINTN) CurrentImageBlock - (UINTN) ImagePackage->ImageBlock);
  Part2Size = ImagePackage->ImageBlockSize - Part1Size - OldBlockSize;
  CopyMem (ImageBlocks, ImagePackage->ImageBlock, Part1Size);

  //
  // Set the new image block
  //
  NewImageBlock = (EFI_HII_IMAGE_BLOCK *) ((UINT8 *) ImageBlocks + Part1Size);
  if ((Image->Flags & EFI_IMAGE_TRANSPARENT) == EFI_IMAGE_TRANSPARENT) {
    NewImageBlock->BlockType= EFI_HII_IIBT_IMAGE_24BIT_TRANS;
  } else {
    NewImageBlock->BlockType = EFI_HII_IIBT_IMAGE_24BIT;
  }
  WriteUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) NewImageBlock)->Bitmap.Width, Image->Width);
  WriteUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) NewImageBlock)->Bitmap.Height, Image->Height);
  CopyGopToRgbPixel (((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) NewImageBlock)->Bitmap.Bitmap,
                       Image->Bitmap, (UINT32) Image->Width * Image->Height);

  CopyMem ((UINT8 *) NewImageBlock + NewBlockSize, (UINT8 *) CurrentImageBlock + OldBlockSize, Part2Size);

  FreePool (ImagePackage->ImageBlock);
  ImagePackage->ImageBlock                       = ImageBlocks;
  ImagePackage->ImageBlockSize                  += NewBlockSize - OldBlockSize;
  ImagePackage->ImagePkgHdr.Header.Length       += NewBlockSize - OldBlockSize;
  PackageListNode->PackageListHdr.PackageLength += NewBlockSize - OldBlockSize;

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