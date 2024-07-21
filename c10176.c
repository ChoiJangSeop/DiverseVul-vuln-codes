MHD_destroy_post_processor (struct MHD_PostProcessor *pp)
{
  enum MHD_Result ret;

  if (NULL == pp)
    return MHD_YES;
  if (PP_ProcessValue == pp->state)
  {
    /* key without terminated value left at the end of the
       buffer; fake receiving a termination character to
       ensure it is also processed */
    post_process_urlencoded (pp,
                             "\n",
                             1);
  }
  /* These internal strings need cleaning up since
     the post-processing may have been interrupted
     at any stage */
  if ( (pp->xbuf_pos > 0) ||
       (pp->state != PP_Done) )
    ret = MHD_NO;
  else
    ret = MHD_YES;
  pp->have = NE_none;
  free_unmarked (pp);
  if (NULL != pp->nested_boundary)
    free (pp->nested_boundary);
  free (pp);
  return ret;
}