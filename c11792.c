start_pass_merged_upsample(j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample = (my_upsample_ptr)cinfo->upsample;

  /* Mark the spare buffer empty */
  upsample->spare_full = FALSE;
  /* Initialize total-height counter for detecting bottom of image */
  upsample->rows_to_go = cinfo->output_height;
}