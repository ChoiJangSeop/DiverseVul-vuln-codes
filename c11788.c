void LibRaw::parseSonySR2(uchar *cbuf_SR2, unsigned SR2SubIFDOffset,
                          unsigned SR2SubIFDLength, unsigned dng_writer)
{
  unsigned c;
  unsigned entries, tag_id, tag_type, tag_datalen;
  INT64 sr2_offset, tag_offset, tag_data, tag_dataoffset;
  int TagProcessed;
  int tag_dataunitlen;
  float num;
  int i;
  int WBCTC_count;
  entries = sget2(cbuf_SR2);
  if (entries > 1000)
    return;
  tag_offset = 2;
  WBCTC_count = 0;
  while (entries--) {
    if (tiff_sget (SR2SubIFDOffset, cbuf_SR2, SR2SubIFDLength,
                   &tag_offset, &tag_id, &tag_type, &tag_dataoffset,
                   &tag_datalen, &tag_dataunitlen) == 0) {
      TagProcessed = 0;
      if (dng_writer == nonDNG) {
        switch (tag_id) {
        case 0x7300:
          FORC4 cblack[c] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
          TagProcessed = 1;
          break;
        case 0x7303:
          FORC4 cam_mul[GRBG_2_RGBG(c)] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
          TagProcessed = 1;
          break;
        case 0x7310:
          FORC4 cblack[RGGB_2_RGBG(c)] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
          i = cblack[3];
          FORC3 if (i > cblack[c]) i = cblack[c];
          FORC4 cblack[c] -= i;
          black = i;
          TagProcessed = 1;
          break;
        case 0x7313:
          FORC4 cam_mul[RGGB_2_RGBG(c)] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
          TagProcessed = 1;
          break;
        case 0x74a0:
          ilm.MaxAp4MaxFocal = sgetreal(tag_type, cbuf_SR2 + tag_dataoffset);
          TagProcessed = 1;
          break;
        case 0x74a1:
          ilm.MaxAp4MinFocal = sgetreal(tag_type, cbuf_SR2 + tag_dataoffset);
          TagProcessed = 1;
          break;
        case 0x74a2:
          ilm.MaxFocal = sgetreal(tag_type, cbuf_SR2 + tag_dataoffset);
          TagProcessed = 1;
          break;
        case 0x74a3:
          ilm.MinFocal = sgetreal(tag_type, cbuf_SR2 + tag_dataoffset);
          TagProcessed = 1;
          break;
        case 0x7800:
          for (i = 0; i < 3; i++)
          {
            num = 0.0;
            for (c = 0; c < 3; c++)
            {
              imgdata.color.ccm[i][c] =
                  (float)((short)sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * (i * 3 + c)));
              num += imgdata.color.ccm[i][c];
            }
            if (num > 0.01)
              FORC3 imgdata.color.ccm[i][c] = imgdata.color.ccm[i][c] / num;
          }
          TagProcessed = 1;
          break;
        case 0x787f:
          if (tag_datalen == 3)
          {
            FORC3 imgdata.color.linear_max[c] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
            imgdata.color.linear_max[3] = imgdata.color.linear_max[1];
          }
          else if (tag_datalen == 1)
          {
            imgdata.color.linear_max[0] = imgdata.color.linear_max[1] =
                imgdata.color.linear_max[2] = imgdata.color.linear_max[3] =
                    sget2(cbuf_SR2 + tag_dataoffset);
          }
          TagProcessed = 1;
          break;
        }
      }

      if (!TagProcessed) {
        if ((tag_id >= 0x7480) && (tag_id <= 0x7486)) {
          i = tag_id - 0x7480;
          if (Sony_SR2_wb_list[i] > 255) {
            icWBCCTC[WBCTC_count][0] = Sony_SR2_wb_list[i];
            FORC3 icWBCCTC[WBCTC_count][c + 1] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
            icWBCCTC[WBCTC_count][4] = icWBCCTC[WBCTC_count][2];
            WBCTC_count++;
          } else {
            FORC3 icWBC[Sony_SR2_wb_list[i]][c] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
            icWBC[Sony_SR2_wb_list[i]][3] = icWBC[Sony_SR2_wb_list[i]][1];
          }
        } else if ((tag_id >= 0x7820) && (tag_id <= 0x782d)) {
          i = tag_id - 0x7820;
          if (Sony_SR2_wb_list1[i] > 255) {
            icWBCCTC[WBCTC_count][0] = Sony_SR2_wb_list1[i];
            FORC3 icWBCCTC[WBCTC_count][c + 1] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
            icWBCCTC[WBCTC_count][4] = icWBCCTC[WBCTC_count][2];
            if (Sony_SR2_wb_list1[i] == 3200) {
              FORC3 icWBC[LIBRAW_WBI_StudioTungsten][c] = icWBCCTC[WBCTC_count][c + 1];
              icWBC[LIBRAW_WBI_StudioTungsten][3] = icWBC[LIBRAW_WBI_StudioTungsten][1];
            }
            WBCTC_count++;
          } else {
            FORC3 icWBC[Sony_SR2_wb_list1[i]][c] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
            icWBC[Sony_SR2_wb_list1[i]][3] = icWBC[Sony_SR2_wb_list1[i]][1];
          }
        } else if (tag_id == 0x7302) {
          FORC4 icWBC[LIBRAW_WBI_Auto][GRBG_2_RGBG(c)] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
        } else if (tag_id == 0x7312) {
          FORC4 icWBC[LIBRAW_WBI_Auto][RGGB_2_RGBG(c)] = sget2(cbuf_SR2 + tag_dataoffset + tag_dataunitlen * c);
        }
      }
    }
  }
}