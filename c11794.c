build_ycc_rgb_table(j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample = (my_upsample_ptr)cinfo->upsample;
  int i;
  JLONG x;
  SHIFT_TEMPS

  upsample->Cr_r_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE + 1) * sizeof(int));
  upsample->Cb_b_tab = (int *)
    (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE + 1) * sizeof(int));
  upsample->Cr_g_tab = (JLONG *)
    (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE + 1) * sizeof(JLONG));
  upsample->Cb_g_tab = (JLONG *)
    (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_IMAGE,
                                (MAXJSAMPLE + 1) * sizeof(JLONG));

  for (i = 0, x = -CENTERJSAMPLE; i <= MAXJSAMPLE; i++, x++) {
    /* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
    /* The Cb or Cr value we are thinking of is x = i - CENTERJSAMPLE */
    /* Cr=>R value is nearest int to 1.40200 * x */
    upsample->Cr_r_tab[i] = (int)
                    RIGHT_SHIFT(FIX(1.40200) * x + ONE_HALF, SCALEBITS);
    /* Cb=>B value is nearest int to 1.77200 * x */
    upsample->Cb_b_tab[i] = (int)
                    RIGHT_SHIFT(FIX(1.77200) * x + ONE_HALF, SCALEBITS);
    /* Cr=>G value is scaled-up -0.71414 * x */
    upsample->Cr_g_tab[i] = (-FIX(0.71414)) * x;
    /* Cb=>G value is scaled-up -0.34414 * x */
    /* We also add in ONE_HALF so that need not do it in inner loop */
    upsample->Cb_g_tab[i] = (-FIX(0.34414)) * x + ONE_HALF;
  }
}