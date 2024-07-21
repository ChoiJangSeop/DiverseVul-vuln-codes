TEST(DecodeAPI, Vp9PeekSI) {
  const vpx_codec_iface_t *const codec = &vpx_codec_vp9_dx_algo;
  // The first 9 bytes are valid and the rest of the bytes are made up. Until
  // size 10, this should return VPX_CODEC_UNSUP_BITSTREAM and after that it
  // should return VPX_CODEC_CORRUPT_FRAME.
  const uint8_t data[32] = {
    0x85, 0xa4, 0xc1, 0xa1, 0x38, 0x81, 0xa3, 0x49, 0x83, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  };

  for (uint32_t data_sz = 1; data_sz <= 32; ++data_sz) {
    // Verify behavior of vpx_codec_decode. vpx_codec_decode doesn't even get
    // to decoder_peek_si_internal on frames of size < 8.
    if (data_sz >= 8) {
      vpx_codec_ctx_t dec;
      EXPECT_EQ(VPX_CODEC_OK, vpx_codec_dec_init(&dec, codec, NULL, 0));
      EXPECT_EQ(
          (data_sz < 10) ? VPX_CODEC_UNSUP_BITSTREAM : VPX_CODEC_CORRUPT_FRAME,
          vpx_codec_decode(&dec, data, data_sz, NULL, 0));
      vpx_codec_iter_t iter = NULL;
      EXPECT_EQ(NULL, vpx_codec_get_frame(&dec, &iter));
      EXPECT_EQ(VPX_CODEC_OK, vpx_codec_destroy(&dec));
    }

    // Verify behavior of vpx_codec_peek_stream_info.
    vpx_codec_stream_info_t si;
    si.sz = sizeof(si);
    EXPECT_EQ((data_sz < 10) ? VPX_CODEC_UNSUP_BITSTREAM : VPX_CODEC_OK,
              vpx_codec_peek_stream_info(codec, data, data_sz, &si));
  }
}