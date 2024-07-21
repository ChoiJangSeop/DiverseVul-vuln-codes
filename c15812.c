void apply_sample_adaptive_offset_sequential(de265_image* img)
{
  const seq_parameter_set& sps = img->get_sps();

  if (sps.sample_adaptive_offset_enabled_flag==0) {
    return;
  }

  int lumaImageSize   = img->get_image_stride(0) * img->get_height(0) * img->get_bytes_per_pixel(0);
  int chromaImageSize = img->get_image_stride(1) * img->get_height(1) * img->get_bytes_per_pixel(1);

  uint8_t* inputCopy = new uint8_t[ libde265_max(lumaImageSize, chromaImageSize) ];
  if (inputCopy == NULL) {
    img->decctx->add_warning(DE265_WARNING_CANNOT_APPLY_SAO_OUT_OF_MEMORY,false);
    return;
  }


  int nChannels = 3;
  if (sps.ChromaArrayType == CHROMA_MONO) { nChannels=1; }

  for (int cIdx=0;cIdx<nChannels;cIdx++) {

    int stride = img->get_image_stride(cIdx);
    int height = img->get_height(cIdx);

    memcpy(inputCopy, img->get_image_plane(cIdx), stride * height * img->get_bytes_per_pixel(cIdx));

    for (int yCtb=0; yCtb<sps.PicHeightInCtbsY; yCtb++)
      for (int xCtb=0; xCtb<sps.PicWidthInCtbsY; xCtb++)
        {
          const slice_segment_header* shdr = img->get_SliceHeaderCtb(xCtb,yCtb);
          if (shdr==NULL) { return; }

          if (cIdx==0 && shdr->slice_sao_luma_flag) {
            apply_sao(img, xCtb,yCtb, shdr, 0, 1<<sps.Log2CtbSizeY, 1<<sps.Log2CtbSizeY,
                      inputCopy, stride,
                      img->get_image_plane(0), img->get_image_stride(0));
          }

          if (cIdx!=0 && shdr->slice_sao_chroma_flag) {
            int nSW = (1<<sps.Log2CtbSizeY) / sps.SubWidthC;
            int nSH = (1<<sps.Log2CtbSizeY) / sps.SubHeightC;

            apply_sao(img, xCtb,yCtb, shdr, cIdx, nSW,nSH,
                      inputCopy, stride,
                      img->get_image_plane(cIdx), img->get_image_stride(cIdx));
          }
        }
  }

  delete[] inputCopy;
}