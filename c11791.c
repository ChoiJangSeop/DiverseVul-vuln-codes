merged_1v_upsample(j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                   JDIMENSION *in_row_group_ctr,
                   JDIMENSION in_row_groups_avail, JSAMPARRAY output_buf,
                   JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail)
/* 1:1 vertical sampling case: much easier, never need a spare row. */
{
  my_upsample_ptr upsample = (my_upsample_ptr)cinfo->upsample;

  /* Just do the upsampling. */
  (*upsample->upmethod) (cinfo, input_buf, *in_row_group_ctr,
                         output_buf + *out_row_ctr);
  /* Adjust counts */
  (*out_row_ctr)++;
  (*in_row_group_ctr)++;
}