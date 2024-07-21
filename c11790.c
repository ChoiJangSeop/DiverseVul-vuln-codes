merged_2v_upsample(j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                   JDIMENSION *in_row_group_ctr,
                   JDIMENSION in_row_groups_avail, JSAMPARRAY output_buf,
                   JDIMENSION *out_row_ctr, JDIMENSION out_rows_avail)
/* 2:1 vertical sampling case: may need a spare row. */
{
  my_upsample_ptr upsample = (my_upsample_ptr)cinfo->upsample;
  JSAMPROW work_ptrs[2];
  JDIMENSION num_rows;          /* number of rows returned to caller */

  if (upsample->spare_full) {
    /* If we have a spare row saved from a previous cycle, just return it. */
    JDIMENSION size = upsample->out_row_width;
    if (cinfo->out_color_space == JCS_RGB565)
      size = cinfo->output_width * 2;
    jcopy_sample_rows(&upsample->spare_row, 0, output_buf + *out_row_ctr, 0, 1,
                      size);
    num_rows = 1;
    upsample->spare_full = FALSE;
  } else {
    /* Figure number of rows to return to caller. */
    num_rows = 2;
    /* Not more than the distance to the end of the image. */
    if (num_rows > upsample->rows_to_go)
      num_rows = upsample->rows_to_go;
    /* And not more than what the client can accept: */
    out_rows_avail -= *out_row_ctr;
    if (num_rows > out_rows_avail)
      num_rows = out_rows_avail;
    /* Create output pointer array for upsampler. */
    work_ptrs[0] = output_buf[*out_row_ctr];
    if (num_rows > 1) {
      work_ptrs[1] = output_buf[*out_row_ctr + 1];
    } else {
      work_ptrs[1] = upsample->spare_row;
      upsample->spare_full = TRUE;
    }
    /* Now do the upsampling. */
    (*upsample->upmethod) (cinfo, input_buf, *in_row_group_ctr, work_ptrs);
  }

  /* Adjust counts */
  *out_row_ctr += num_rows;
  upsample->rows_to_go -= num_rows;
  /* When the buffer is emptied, declare this input row group consumed */
  if (!upsample->spare_full)
    (*in_row_group_ctr)++;
}