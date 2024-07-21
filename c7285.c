HiiDrawImage (
  IN CONST EFI_HII_IMAGE_PROTOCOL    *This,
  IN EFI_HII_DRAW_FLAGS              Flags,
  IN CONST EFI_IMAGE_INPUT           *Image,
  IN OUT EFI_IMAGE_OUTPUT            **Blt,
  IN UINTN                           BltX,
  IN UINTN                           BltY
  )
{
  EFI_STATUS                          Status;
  HII_DATABASE_PRIVATE_DATA           *Private;
  BOOLEAN                             Transparent;
  EFI_IMAGE_OUTPUT                    *ImageOut;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *BltBuffer;
  UINTN                               BufferLen;
  UINTN                               Width;
  UINTN                               Height;
  UINTN                               Xpos;
  UINTN                               Ypos;
  UINTN                               OffsetY1;
  UINTN                               OffsetY2;
  EFI_FONT_DISPLAY_INFO               *FontInfo;
  UINTN                               Index;

  if (This == NULL || Image == NULL || Blt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Flags & EFI_HII_DRAW_FLAG_CLIP) == EFI_HII_DRAW_FLAG_CLIP && *Blt == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Flags & EFI_HII_DRAW_FLAG_TRANSPARENT) == EFI_HII_DRAW_FLAG_TRANSPARENT) {
    return EFI_INVALID_PARAMETER;
  }

  FontInfo = NULL;

  //
  // Check whether the image will be drawn transparently or opaquely.
  //
  Transparent = FALSE;
  if ((Flags & EFI_HII_DRAW_FLAG_TRANSPARENT) == EFI_HII_DRAW_FLAG_FORCE_TRANS) {
    Transparent = TRUE;
  } else if ((Flags & EFI_HII_DRAW_FLAG_TRANSPARENT) == EFI_HII_DRAW_FLAG_FORCE_OPAQUE){
    Transparent = FALSE;
  } else {
    //
    // Now EFI_HII_DRAW_FLAG_DEFAULT is set, whether image will be drawn depending
    // on the image's transparency setting.
    //
    if ((Image->Flags & EFI_IMAGE_TRANSPARENT) == EFI_IMAGE_TRANSPARENT) {
      Transparent = TRUE;
    }
  }

  //
  // Image cannot be drawn transparently if Blt points to NULL on entry.
  // Currently output to Screen transparently is not supported, either.
  //
  if (Transparent) {
    if (*Blt == NULL) {
      return EFI_INVALID_PARAMETER;
    } else if ((Flags & EFI_HII_DIRECT_TO_SCREEN) == EFI_HII_DIRECT_TO_SCREEN) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Private = HII_IMAGE_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  //
  // When Blt points to a non-NULL on entry, this image will be drawn onto
  // this bitmap or screen pointed by "*Blt" and EFI_HII_DRAW_FLAG_CLIP is implied.
  // Otherwise a new bitmap will be allocated to hold this image.
  //
  if (*Blt != NULL) {
    //
    // Clip the image by (Width, Height)
    //

    Width  = Image->Width;
    Height = Image->Height;

    if (Width > (*Blt)->Width - BltX) {
      Width = (*Blt)->Width - BltX;
    }
    if (Height > (*Blt)->Height - BltY) {
      Height = (*Blt)->Height - BltY;
    }

    BufferLen = Width * Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    BltBuffer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) AllocateZeroPool (BufferLen);
    if (BltBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (Width == Image->Width && Height == Image->Height) {
      CopyMem (BltBuffer, Image->Bitmap, BufferLen);
    } else {
      for (Ypos = 0; Ypos < Height; Ypos++) {
        OffsetY1 = Image->Width * Ypos;
        OffsetY2 = Width * Ypos;
        for (Xpos = 0; Xpos < Width; Xpos++) {
          BltBuffer[OffsetY2 + Xpos] = Image->Bitmap[OffsetY1 + Xpos];
        }
      }
    }

    //
    // Draw the image to existing bitmap or screen depending on flag.
    //
    if ((Flags & EFI_HII_DIRECT_TO_SCREEN) == EFI_HII_DIRECT_TO_SCREEN) {
      //
      // Caller should make sure the current UGA console is grarphic mode.
      //

      //
      // Write the image directly to the output device specified by Screen.
      //
      Status = (*Blt)->Image.Screen->Blt (
                                       (*Blt)->Image.Screen,
                                       BltBuffer,
                                       EfiBltBufferToVideo,
                                       0,
                                       0,
                                       BltX,
                                       BltY,
                                       Width,
                                       Height,
                                       0
                                       );
    } else {
      //
      // Draw the image onto the existing bitmap specified by Bitmap.
      //
      Status = ImageToBlt (
                 BltBuffer,
                 BltX,
                 BltY,
                 Width,
                 Height,
                 Transparent,
                 Blt
                 );

    }

    FreePool (BltBuffer);
    return Status;

  } else {
    //
    // Allocate a new bitmap to hold the incoming image.
    //
    Width  = Image->Width  + BltX;
    Height = Image->Height + BltY;

    BufferLen = Width * Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    BltBuffer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) AllocateZeroPool (BufferLen);
    if (BltBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ImageOut = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
    if (ImageOut == NULL) {
      FreePool (BltBuffer);
      return EFI_OUT_OF_RESOURCES;
    }
    ImageOut->Width        = (UINT16) Width;
    ImageOut->Height       = (UINT16) Height;
    ImageOut->Image.Bitmap = BltBuffer;

    //
    // BUGBUG: Now all the "blank" pixels are filled with system default background
    // color. Not sure if it need to be updated or not.
    //
    Status = GetSystemFont (Private, &FontInfo, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (BltBuffer);
      FreePool (ImageOut);
      return Status;
    }
    ASSERT (FontInfo != NULL);
    for (Index = 0; Index < Width * Height; Index++) {
      BltBuffer[Index] = FontInfo->BackgroundColor;
    }
    FreePool (FontInfo);

    //
    // Draw the incoming image to the new created image.
    //
    *Blt = ImageOut;
    return ImageToBlt (
             Image->Bitmap,
             BltX,
             BltY,
             Image->Width,
             Image->Height,
             Transparent,
             Blt
             );

  }
}