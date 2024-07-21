static MagickBooleanType WritePOCKETMODImage(const ImageInfo *image_info,
  Image *image,ExceptionInfo *exception)
{
#define PocketPageOrder  "1,2,3,4,0,7,6,5"

  const Image
    *next;

  Image
    *pages,
    *pocket_mod;

  MagickBooleanType
    status;

  register ssize_t
    i;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  pocket_mod=NewImageList();
  pages=NewImageList();
  i=0;
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    Image
      *page;

    if ((i == 0) || (i == 5) || (i == 6) || (i == 7))
      page=RotateImage(next,180.0,exception);
    else
      page=CloneImage(next,0,0,MagickTrue,exception);
    if (page == (Image *) NULL)
      break;
    (void) SetImageAlphaChannel(page,RemoveAlphaChannel,exception);
    page->scene=i++;
    AppendImageToList(&pages,page);
    if ((i == 8) || (GetNextImageInList(next) == (Image *) NULL))
      {
        Image
          *images,
          *page_layout;

        MontageInfo
          *montage_info;

        /*
          Create PocketMod page.
        */
        for (i=(ssize_t) GetImageListLength(pages); i < 8; i++)
        {
          page=CloneImage(pages,0,0,MagickTrue,exception);
          (void) QueryColorCompliance("#FFF",AllCompliance,
            &page->background_color,exception);
          SetImageBackgroundColor(page,exception);
          page->scene=i;
          AppendImageToList(&pages,page);
        }
        images=CloneImages(pages,PocketPageOrder,exception);
        pages=DestroyImageList(pages);
        if (images == (Image *) NULL)
          break;
        montage_info=CloneMontageInfo(image_info,(MontageInfo *) NULL);
        (void) CloneString(&montage_info->geometry,"877x1240+0+0");
        (void) CloneString(&montage_info->tile,"4x2");
        (void) QueryColorCompliance("#000",AllCompliance,
          &montage_info->border_color,exception);
        montage_info->border_width=2;
        page_layout=MontageImages(images,montage_info,exception);
        montage_info=DestroyMontageInfo(montage_info);
        images=DestroyImageList(images);
        if (page_layout == (Image *) NULL)
          break;
        AppendImageToList(&pocket_mod,page_layout);
        i=0;
      }
  }
  if (pocket_mod == (Image *) NULL)
    return(MagickFalse);
  status=WritePDFImage(image_info,GetFirstImageInList(pocket_mod),exception);
  pocket_mod=DestroyImageList(pocket_mod);
  return(status);
}