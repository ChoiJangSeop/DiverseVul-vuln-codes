Output4bitPixel (
  IN OUT EFI_IMAGE_INPUT             *Image,
  IN UINT8                           *Data,
  IN EFI_HII_IMAGE_PALETTE_INFO      *PaletteInfo
  )
{
  UINT16                             Xpos;
  UINT16                             Ypos;
  UINTN                              OffsetY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BitMapPtr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      PaletteValue[16];
  EFI_HII_IMAGE_PALETTE_INFO         *Palette;
  UINTN                              PaletteSize;
  UINT16                             PaletteNum;
  UINT8                              Byte;

  ASSERT (Image != NULL && Data != NULL && PaletteInfo != NULL);

  BitMapPtr = Image->Bitmap;

  //
  // The bitmap should allocate each color index starting from 0.
  //
  PaletteSize = 0;
  CopyMem (&PaletteSize, PaletteInfo, sizeof (UINT16));
  PaletteSize += sizeof (UINT16);
  Palette = AllocateZeroPool (PaletteSize);
  ASSERT (Palette != NULL);
  if (Palette == NULL) {
    return;
  }
  CopyMem (Palette, PaletteInfo, PaletteSize);
  PaletteNum = (UINT16)(Palette->PaletteSize / sizeof (EFI_HII_RGB_PIXEL));

  ZeroMem (PaletteValue, sizeof (PaletteValue));
  CopyRgbToGopPixel (PaletteValue, Palette->PaletteValue, PaletteNum);
  FreePool (Palette);

  //
  // Convert the pixel from 4 bit to corresponding color.
  //
  for (Ypos = 0; Ypos < Image->Height; Ypos++) {
    OffsetY = BITMAP_LEN_4_BIT (Image->Width, Ypos);
    //
    // All bits in these bytes are meaningful
    //
    for (Xpos = 0; Xpos < Image->Width / 2; Xpos++) {
      Byte = *(Data + OffsetY + Xpos);
      BitMapPtr[Ypos * Image->Width + Xpos * 2]     = PaletteValue[Byte >> 4];
      BitMapPtr[Ypos * Image->Width + Xpos * 2 + 1] = PaletteValue[Byte & 0x0F];
    }

    if (Image->Width % 2 != 0) {
      //
      // Padding bits in this byte should be ignored.
      //
      Byte = *(Data + OffsetY + Xpos);
      BitMapPtr[Ypos * Image->Width + Xpos * 2]     = PaletteValue[Byte >> 4];
    }
  }
}