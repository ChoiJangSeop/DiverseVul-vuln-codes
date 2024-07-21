void CLASS parse_fuji(int offset)
{
  unsigned entries, tag, len, save, c;

  fseek(ifp, offset, SEEK_SET);
  entries = get4();
  if (entries > 255)
    return;
  while (entries--)
  {
    tag = get2();
    len = get2();
    save = ftell(ifp);

    if (tag == 0x100)
    {
      raw_height = get2();
      raw_width = get2();
    }
    else if (tag == 0x121)
    {
      height = get2();
      if ((width = get2()) == 4284)
        width += 3;
    }
    else if (tag == 0x130)
    {
      fuji_layout = fgetc(ifp) >> 7;
      fuji_width = !(fgetc(ifp) & 8);
    }
    else if (tag == 0x131)
    {
      filters = 9;
      FORC(36)
        {
	   int q = fgetc(ifp);
	   xtrans_abs[0][35 - c] = MAX(0,MIN(q,2)); /* & 3;*/
	}
    }
    else if (tag == 0x2ff0)
    {
      FORC4 cam_mul[c ^ 1] = get2();

// IB start
#ifdef LIBRAW_LIBRARY_BUILD
    }
    else if (tag == 0x9650)
    {
      short a = (short)get2();
      float b = fMAX(1.0f, get2());
      imgdata.makernotes.fuji.FujiExpoMidPointShift = a / b;
    }

    else if (tag == 0x2f00)
    {
      int nWBs = get4();
      nWBs = MIN(nWBs, 6);
      for (int wb_ind = 0; wb_ind < nWBs; wb_ind++)
      {
        FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Custom1 + wb_ind][c ^ 1] = get2();
        fseek(ifp, 8, SEEK_CUR);
      }
    }

    else if (tag == 0x2000)
    {
      FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Auto][c ^ 1] = get2();
    }
    else if (tag == 0x2100)
    {
      FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FineWeather][c ^ 1] = get2();
    }
    else if (tag == 0x2200)
    {
      FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Shade][c ^ 1] = get2();
    }
    else if (tag == 0x2300)
    {
      FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_D][c ^ 1] = get2();
    }
    else if (tag == 0x2301)
    {
      FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_N][c ^ 1] = get2();
    }
    else if (tag == 0x2302)
    {
      FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_WW][c ^ 1] = get2();
    }
    else if (tag == 0x2310)
    {
      FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_L][c ^ 1] = get2();
    }
    else if (tag == 0x2400)
    {
      FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Tungsten][c ^ 1] = get2();
    }
    else if (tag == 0x2410)
    {
      FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Flash][c ^ 1] = get2();
#endif
      // IB end
    }
    else if (tag == 0xc000)
    /* 0xc000 tag versions, second ushort; valid if the first ushort is 0
    X100F	0x0259
    X100T	0x0153
    X-E2	0x014f	0x024f depends on firmware
    X-A1	0x014e
    XQ2		0x0150
    XQ1		0x0150
    X100S	0x0149	0x0249 depends on firmware
    X30		0x0152
    X20		0x0146
    X-T10	0x0154
    X-T2	0x0258
    X-M1	0x014d
    X-E2s	0x0355
    X-A2	0x014e
    X-T20	0x025b
    GFX 50S	0x025a
    X-T1	0x0151	0x0251 0x0351 depends on firmware
    X70		0x0155
    X-Pro2	0x0255
    */
    {
      c = order;
      order = 0x4949;
      if ((tag = get4()) > 10000)
        tag = get4();
      if (tag > 10000)
        tag = get4();
      width = tag;
      height = get4();
#ifdef LIBRAW_LIBRARY_BUILD
      if (!strcmp(model, "X-A3") || !strcmp(model, "X-A10"))
      {
        int wb[4];
        int nWB, tWB, pWB;
        int iCCT = 0;
        int cnt;
        fseek(ifp, save + 0x200, SEEK_SET);
        for (int wb_ind = 0; wb_ind < 42; wb_ind++)
        {
          nWB = get4();
          tWB = get4();
          wb[0] = get4() << 1;
          wb[1] = get4();
          wb[3] = get4();
          wb[2] = get4() << 1;
          if (tWB && (iCCT < 255))
          {
            imgdata.color.WBCT_Coeffs[iCCT][0] = tWB;
            for (cnt = 0; cnt < 4; cnt++)
              imgdata.color.WBCT_Coeffs[iCCT][cnt + 1] = wb[cnt];
            iCCT++;
          }
          if (nWB != 70)
          {
            for (pWB = 1; pWB < nFuji_wb_list2; pWB += 2)
            {
              if (Fuji_wb_list2[pWB] == nWB)
              {
                for (cnt = 0; cnt < 4; cnt++)
                  imgdata.color.WB_Coeffs[Fuji_wb_list2[pWB - 1]][cnt] = wb[cnt];
                break;
              }
            }
          }
        }
      }
      else
      {
        libraw_internal_data.unpacker_data.posRAFData = save;
        libraw_internal_data.unpacker_data.lenRAFData = (len >> 1);
      }
#endif
      order = c;
    }
    fseek(ifp, save + len, SEEK_SET);
  }
  height <<= fuji_layout;
  width >>= fuji_layout;
}