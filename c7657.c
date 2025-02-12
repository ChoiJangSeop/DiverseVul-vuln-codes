MagickExport void RemoveDuplicateLayers(Image **images,
     ExceptionInfo *exception)
{
  register Image
    *curr,
    *next;

  RectangleInfo
    bounds;

  assert((*images) != (const Image *) NULL);
  assert((*images)->signature == MagickCoreSignature);
  if ((*images)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*images)->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);

  curr=GetFirstImageInList(*images);
  for (; (next=GetNextImageInList(curr)) != (Image *) NULL; curr=next)
  {
    if ( curr->columns != next->columns || curr->rows != next->rows
         || curr->page.x != next->page.x || curr->page.y != next->page.y )
      continue;
    bounds=CompareImageBounds(curr,next,CompareAnyLayer,exception);
    if ( bounds.x < 0 ) {
      /*
        the two images are the same, merge time delays and delete one.
      */
      size_t time;
      time = curr->delay*1000/curr->ticks_per_second;
      time += next->delay*1000/next->ticks_per_second;
      next->ticks_per_second = 100L;
      next->delay = time*curr->ticks_per_second/1000;
      next->iterations = curr->iterations;
      *images = curr;
      (void) DeleteImageFromList(images);
    }
  }
  *images = GetFirstImageInList(*images);
}