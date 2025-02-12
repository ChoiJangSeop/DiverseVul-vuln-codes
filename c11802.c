h2v2_merged_upsample_565_internal(j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                                  JDIMENSION in_row_group_ctr,
                                  JSAMPARRAY output_buf)
{
  my_upsample_ptr upsample = (my_upsample_ptr)cinfo->upsample;
  register int y, cred, cgreen, cblue;
  int cb, cr;
  register JSAMPROW outptr0, outptr1;
  JSAMPROW inptr00, inptr01, inptr1, inptr2;
  JDIMENSION col;
  /* copy these pointers into registers if possible */
  register JSAMPLE *range_limit = cinfo->sample_range_limit;
  int *Crrtab = upsample->Cr_r_tab;
  int *Cbbtab = upsample->Cb_b_tab;
  JLONG *Crgtab = upsample->Cr_g_tab;
  JLONG *Cbgtab = upsample->Cb_g_tab;
  unsigned int r, g, b;
  JLONG rgb;
  SHIFT_TEMPS

  inptr00 = input_buf[0][in_row_group_ctr * 2];
  inptr01 = input_buf[0][in_row_group_ctr * 2 + 1];
  inptr1 = input_buf[1][in_row_group_ctr];
  inptr2 = input_buf[2][in_row_group_ctr];
  outptr0 = output_buf[0];
  outptr1 = output_buf[1];

  /* Loop for each group of output pixels */
  for (col = cinfo->output_width >> 1; col > 0; col--) {
    /* Do the chroma part of the calculation */
    cb = GETJSAMPLE(*inptr1++);
    cr = GETJSAMPLE(*inptr2++);
    cred = Crrtab[cr];
    cgreen = (int)RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];

    /* Fetch 4 Y values and emit 4 pixels */
    y  = GETJSAMPLE(*inptr00++);
    r = range_limit[y + cred];
    g = range_limit[y + cgreen];
    b = range_limit[y + cblue];
    rgb = PACK_SHORT_565(r, g, b);

    y  = GETJSAMPLE(*inptr00++);
    r = range_limit[y + cred];
    g = range_limit[y + cgreen];
    b = range_limit[y + cblue];
    rgb = PACK_TWO_PIXELS(rgb, PACK_SHORT_565(r, g, b));

    WRITE_TWO_PIXELS(outptr0, rgb);
    outptr0 += 4;

    y  = GETJSAMPLE(*inptr01++);
    r = range_limit[y + cred];
    g = range_limit[y + cgreen];
    b = range_limit[y + cblue];
    rgb = PACK_SHORT_565(r, g, b);

    y  = GETJSAMPLE(*inptr01++);
    r = range_limit[y + cred];
    g = range_limit[y + cgreen];
    b = range_limit[y + cblue];
    rgb = PACK_TWO_PIXELS(rgb, PACK_SHORT_565(r, g, b));

    WRITE_TWO_PIXELS(outptr1, rgb);
    outptr1 += 4;
  }

  /* If image width is odd, do the last output column separately */
  if (cinfo->output_width & 1) {
    cb = GETJSAMPLE(*inptr1);
    cr = GETJSAMPLE(*inptr2);
    cred = Crrtab[cr];
    cgreen = (int)RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr], SCALEBITS);
    cblue = Cbbtab[cb];

    y  = GETJSAMPLE(*inptr00);
    r = range_limit[y + cred];
    g = range_limit[y + cgreen];
    b = range_limit[y + cblue];
    rgb = PACK_SHORT_565(r, g, b);
    *(INT16 *)outptr0 = (INT16)rgb;

    y  = GETJSAMPLE(*inptr01);
    r = range_limit[y + cred];
    g = range_limit[y + cgreen];
    b = range_limit[y + cblue];
    rgb = PACK_SHORT_565(r, g, b);
    *(INT16 *)outptr1 = (INT16)rgb;
  }
}