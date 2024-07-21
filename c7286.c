IGetImage (
  IN  LIST_ENTRY                     *Database,
  IN  EFI_HII_HANDLE                 PackageList,
  IN  EFI_IMAGE_ID                   ImageId,
  OUT EFI_IMAGE_INPUT                *Image,
  IN  BOOLEAN                        BitmapOnly
  )
{
  EFI_STATUS                          Status;
  HII_DATABASE_PACKAGE_LIST_INSTANCE  *PackageListNode;
  HII_IMAGE_PACKAGE_INSTANCE          *ImagePackage;
  EFI_HII_IMAGE_BLOCK                 *CurrentImageBlock;
  EFI_HII_IIBT_IMAGE_1BIT_BLOCK       Iibt1bit;
  UINT16                              Width;
  UINT16                              Height;
  UINTN                               ImageLength;
  UINT8                               *PaletteInfo;
  UINT8                               PaletteIndex;
  UINT16                              PaletteSize;
  EFI_HII_IMAGE_DECODER_PROTOCOL      *Decoder;
  EFI_IMAGE_OUTPUT                    *ImageOut;

  if (Image == NULL || ImageId == 0) {
    return EFI_INVALID_PARAMETER;
  }

  PackageListNode = LocatePackageList (Database, PackageList);
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

  Image->Flags = 0;
  switch (CurrentImageBlock->BlockType) {
  case EFI_HII_IIBT_IMAGE_JPEG:
  case EFI_HII_IIBT_IMAGE_PNG:
    if (BitmapOnly) {
      return EFI_UNSUPPORTED;
    }

    ImageOut = NULL;
    Decoder = LocateHiiImageDecoder (CurrentImageBlock->BlockType);
    if (Decoder == NULL) {
      return EFI_UNSUPPORTED;
    }
    //
    // Use the common block code since the definition of two structures is the same.
    //
    ASSERT (OFFSET_OF (EFI_HII_IIBT_JPEG_BLOCK, Data) == OFFSET_OF (EFI_HII_IIBT_PNG_BLOCK, Data));
    ASSERT (sizeof (((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Data) ==
            sizeof (((EFI_HII_IIBT_PNG_BLOCK *) CurrentImageBlock)->Data));
    ASSERT (OFFSET_OF (EFI_HII_IIBT_JPEG_BLOCK, Size) == OFFSET_OF (EFI_HII_IIBT_PNG_BLOCK, Size));
    ASSERT (sizeof (((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Size) ==
            sizeof (((EFI_HII_IIBT_PNG_BLOCK *) CurrentImageBlock)->Size));
    Status = Decoder->DecodeImage (
      Decoder,
      ((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Data,
      ((EFI_HII_IIBT_JPEG_BLOCK *) CurrentImageBlock)->Size,
      &ImageOut,
      FALSE
    );

    //
    // Spec requires to use the first capable image decoder instance.
    // The first image decoder instance may fail to decode the image.
    //
    if (!EFI_ERROR (Status)) {
      Image->Bitmap = ImageOut->Image.Bitmap;
      Image->Height = ImageOut->Height;
      Image->Width = ImageOut->Width;
      FreePool (ImageOut);
    }
    return Status;

  case EFI_HII_IIBT_IMAGE_1BIT_TRANS:
  case EFI_HII_IIBT_IMAGE_4BIT_TRANS:
  case EFI_HII_IIBT_IMAGE_8BIT_TRANS:
    Image->Flags = EFI_IMAGE_TRANSPARENT;
    //
    // fall through
    //
  case EFI_HII_IIBT_IMAGE_1BIT:
  case EFI_HII_IIBT_IMAGE_4BIT:
  case EFI_HII_IIBT_IMAGE_8BIT:
    //
    // Use the common block code since the definition of these structures is the same.
    //
    CopyMem (&Iibt1bit, CurrentImageBlock, sizeof (EFI_HII_IIBT_IMAGE_1BIT_BLOCK));
    ImageLength = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) *
                  ((UINT32) Iibt1bit.Bitmap.Width * Iibt1bit.Bitmap.Height);
    Image->Bitmap = AllocateZeroPool (ImageLength);
    if (Image->Bitmap == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Image->Width  = Iibt1bit.Bitmap.Width;
    Image->Height = Iibt1bit.Bitmap.Height;

    PaletteInfo = ImagePackage->PaletteBlock + sizeof (EFI_HII_IMAGE_PALETTE_INFO_HEADER);
    for (PaletteIndex = 1; PaletteIndex < Iibt1bit.PaletteIndex; PaletteIndex++) {
      CopyMem (&PaletteSize, PaletteInfo, sizeof (UINT16));
      PaletteInfo += PaletteSize + sizeof (UINT16);
    }
    ASSERT (PaletteIndex == Iibt1bit.PaletteIndex);

    //
    // Output bitmap data
    //
    if (CurrentImageBlock->BlockType == EFI_HII_IIBT_IMAGE_1BIT ||
        CurrentImageBlock->BlockType == EFI_HII_IIBT_IMAGE_1BIT_TRANS) {
      Output1bitPixel (
        Image,
        ((EFI_HII_IIBT_IMAGE_1BIT_BLOCK *) CurrentImageBlock)->Bitmap.Data,
        (EFI_HII_IMAGE_PALETTE_INFO *) PaletteInfo
        );
    } else if (CurrentImageBlock->BlockType == EFI_HII_IIBT_IMAGE_4BIT ||
               CurrentImageBlock->BlockType == EFI_HII_IIBT_IMAGE_4BIT_TRANS) {
      Output4bitPixel (
        Image,
        ((EFI_HII_IIBT_IMAGE_4BIT_BLOCK *) CurrentImageBlock)->Bitmap.Data,
        (EFI_HII_IMAGE_PALETTE_INFO *) PaletteInfo
        );
    } else {
      Output8bitPixel (
        Image,
        ((EFI_HII_IIBT_IMAGE_8BIT_BLOCK *) CurrentImageBlock)->Bitmap.Data,
        (EFI_HII_IMAGE_PALETTE_INFO *) PaletteInfo
        );
    }

    return EFI_SUCCESS;

  case EFI_HII_IIBT_IMAGE_24BIT_TRANS:
    Image->Flags = EFI_IMAGE_TRANSPARENT;
    //
    // fall through
    //
  case EFI_HII_IIBT_IMAGE_24BIT:
    Width = ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Width);
    Height = ReadUnaligned16 ((VOID *) &((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Height);
    ImageLength = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * ((UINT32) Width * Height);
    Image->Bitmap = AllocateZeroPool (ImageLength);
    if (Image->Bitmap == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Image->Width  = Width;
    Image->Height = Height;

    //
    // Output the bitmap data directly.
    //
    Output24bitPixel (
      Image,
      ((EFI_HII_IIBT_IMAGE_24BIT_BLOCK *) CurrentImageBlock)->Bitmap.Bitmap
      );
    return EFI_SUCCESS;

  default:
    return EFI_NOT_FOUND;
  }
}