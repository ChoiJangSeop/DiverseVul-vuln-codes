void CLASS parse_makernote_0xc634(int base, int uptag, unsigned dng_writer)
{
  unsigned ver97 = 0, offset = 0, entries, tag, type, len, save, c;
  unsigned i;

  uchar NikonKey, ci, cj, ck;
  unsigned serial = 0;
  unsigned custom_serial = 0;
  unsigned NikonLensDataVersion = 0;
  unsigned lenNikonLensData = 0;
  unsigned NikonFlashInfoVersion = 0;

  uchar *CanonCameraInfo;
  unsigned lenCanonCameraInfo = 0;

  uchar *table_buf;
  uchar *table_buf_0x9050;
  ushort table_buf_0x9050_present = 0;
  uchar *table_buf_0x940c;
  ushort table_buf_0x940c_present = 0;

  short morder, sorder = order;
  char buf[10];
  INT64 fsize = ifp->size();

  fread(buf, 1, 10, ifp);
  if (!strcmp(buf, "Nikon")) {
    base = ftell(ifp);
    order = get2();
    if (get2() != 42) goto quit;
    offset = get4();
    fseek(ifp, offset - 8, SEEK_CUR);
  }
  else if (!strcmp(buf, "OLYMPUS") ||
           !strcmp(buf, "PENTAX ") ||
           (!strncmp(make, "SAMSUNG", 7) && (dng_writer == CameraDNG))) {
    base = ftell(ifp) - 10;
    fseek(ifp, -2, SEEK_CUR);
    order = get2();
    if (buf[0] == 'O') get2();
  }
  else if (!strncmp(buf, "SONY", 4) ||
           !strcmp(buf, "Panasonic")) {
    goto nf;
  }
  else if (!strncmp(buf, "FUJIFILM", 8)) {
    base = ftell(ifp) - 10;
  nf: order = 0x4949;
    fseek(ifp, 2, SEEK_CUR);
  }
  else if (!strcmp(buf, "OLYMP") ||
           !strcmp(buf, "LEICA") ||
           !strcmp(buf, "Ricoh") ||
           !strcmp(buf, "EPSON"))
    fseek(ifp, -2, SEEK_CUR);
  else if (!strcmp(buf, "AOC") ||
           !strcmp(buf, "QVC"))
    fseek(ifp, -4, SEEK_CUR);
  else {
    fseek(ifp, -10, SEEK_CUR);
    if ((!strncmp(make, "SAMSUNG", 7) &&
				(dng_writer == AdobeDNG)))
      base = ftell(ifp);
  }

  entries = get2();
  if (entries > 1000) return;
  morder = order;

  while (entries--) {
    order = morder;
    tiff_get(base, &tag, &type, &len, &save);
    INT64 pos = ifp->tell();
    if(len > 8 && pos+len > 2* fsize) continue;
    tag |= uptag << 16;
    if(len > 100*1024*1024) goto next; // 100Mb tag? No!

    if (!strncmp(make, "Canon",5))
      {
        if (tag == 0x000d && len < 256000) // camera info
          {
            CanonCameraInfo = (uchar*)malloc(len);
            fread(CanonCameraInfo, len, 1, ifp);
            lenCanonCameraInfo = len;
          }

        else if (tag == 0x10)	 // Canon ModelID
          {
            unique_id = get4();
            if (unique_id == 0x03740000) unique_id = 0x80000374;	// M3
            if (unique_id == 0x03840000) unique_id = 0x80000384;	// M10
            if (unique_id == 0x03940000) unique_id = 0x80000394;	// M5
            setCanonBodyFeatures(unique_id);
            if (lenCanonCameraInfo)
              {
                processCanonCameraInfo(unique_id, CanonCameraInfo,lenCanonCameraInfo);
                free(CanonCameraInfo);
                CanonCameraInfo = 0;
                lenCanonCameraInfo = 0;
              }
          }

        else parseCanonMakernotes (tag, type, len);
      }

    else if (!strncmp(make, "FUJI", 4))
      parseFujiMakernotes (tag, type);

    else if (!strncasecmp(make, "LEICA", 5))
      {
        if (((tag == 0x035e) || (tag == 0x035f)) && (type == 10) && (len == 9))
        {
          int ind = tag == 0x035e?0:1;
	      for (int j=0; j < 3; j++)
	       FORCC imgdata.color.dng_color[ind].forwardmatrix[j][c]= getreal(type);
        }
        if ((tag == 0x0303) && (type != 4))
          {
            stmread(imgdata.lens.makernotes.Lens, len,ifp);
          }

        if ((tag == 0x3405) ||
            (tag == 0x0310) ||
            (tag == 0x34003405))
          {
            imgdata.lens.makernotes.LensID = get4();
            imgdata.lens.makernotes.LensID =
              ((imgdata.lens.makernotes.LensID>>2)<<8) |
              (imgdata.lens.makernotes.LensID & 0x3);
            if (imgdata.lens.makernotes.LensID != -1)
              {
                if ((model[0] == 'M') ||
                    !strncasecmp (model, "LEICA M", 7))
                  {
                    imgdata.lens.makernotes.CameraMount = LIBRAW_MOUNT_Leica_M;
                    if (imgdata.lens.makernotes.LensID)
                      imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_Leica_M;
                  }
                else if ((model[0] == 'S') ||
                         !strncasecmp (model, "LEICA S", 7))
                  {
                    imgdata.lens.makernotes.CameraMount = LIBRAW_MOUNT_Leica_S;
                    if (imgdata.lens.makernotes.Lens[0])
                      imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_Leica_S;
                  }
              }
          }

        else if (
                 ((tag == 0x0313) || (tag == 0x34003406)) &&
                 (fabs(imgdata.lens.makernotes.CurAp) < 0.17f) &&
                 ((type == 10) || (type == 5))
                )
          {
            imgdata.lens.makernotes.CurAp = getreal(type);
            if (imgdata.lens.makernotes.CurAp > 126.3)
              imgdata.lens.makernotes.CurAp = 0.0f;
          }

        else if (tag == 0x3400)
          {
            parse_makernote (base, 0x3400);
          }
      }

    else if (!strncmp(make, "NIKON", 5))
      {
        if (tag == 0x1d)							// serial number
          while ((c = fgetc(ifp)) && c != EOF)
          {
            if ((!custom_serial) && (!isdigit(c)))
            {
              if ((strbuflen(model) == 3) && (!strcmp(model,"D50")))
              {
                custom_serial = 34;
              }
              else
              {
                custom_serial = 96;
              }
            }
            serial = serial*10 + (isdigit(c) ? c - '0' : c % 10);
          }
        else if (tag == 0x000a)
          {
            imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_FixedLens;
            imgdata.lens.makernotes.CameraMount = LIBRAW_MOUNT_FixedLens;
          }
        else if (tag == 0x0082)				// lens attachment
          {
            stmread(imgdata.lens.makernotes.Attachment, len, ifp);
          }
        else if (tag == 0x0083)				// lens type
          {
            imgdata.lens.nikon.NikonLensType = fgetc(ifp);
          }
        else if (tag == 0x0084)				// lens
          {
            imgdata.lens.makernotes.MinFocal = getreal(type);
            imgdata.lens.makernotes.MaxFocal = getreal(type);
            imgdata.lens.makernotes.MaxAp4MinFocal = getreal(type);
            imgdata.lens.makernotes.MaxAp4MaxFocal = getreal(type);
          }
        else if (tag == 0x008b)				// lens f-stops
          {
            uchar a, b, c;
            a = fgetc(ifp);
            b = fgetc(ifp);
            c = fgetc(ifp);
            if (c)
              {
                imgdata.lens.nikon.NikonLensFStops = a*b*(12/c);
                imgdata.lens.makernotes.LensFStops =
                  (float)imgdata.lens.nikon.NikonLensFStops /12.0f;
              }
          }
        else if (tag == 0x0093)
          {
            i = get2();
            if ((i == 7) || (i == 9))
            {
              imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_FixedLens;
              imgdata.lens.makernotes.CameraMount = LIBRAW_MOUNT_FixedLens;
            }
          }
        else if (tag == 0x0097)
          {
            for (i=0; i < 4; i++)
	            ver97 = ver97 * 10 + fgetc(ifp)-'0';
	          if (ver97 == 601)  // Coolpix A
	          {
	            imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_FixedLens;
                    imgdata.lens.makernotes.CameraMount = LIBRAW_MOUNT_FixedLens;
	          }
	        }
        else if (tag == 0x0098)				// contains lens data
          {
            for (i = 0; i < 4; i++)
              {
                NikonLensDataVersion = NikonLensDataVersion * 10 + fgetc(ifp) - '0';
              }
            switch (NikonLensDataVersion)
              {
              case 100: lenNikonLensData = 9; break;
              case 101:
              case 201:	// encrypted, starting from v.201
              case 202:
              case 203: lenNikonLensData = 15; break;
              case 204: lenNikonLensData = 16; break;
              case 400: lenNikonLensData = 459; break;
              case 401: lenNikonLensData = 590; break;
              case 402: lenNikonLensData = 509; break;
              case 403: lenNikonLensData = 879; break;
              }
            if(lenNikonLensData)
              {
                table_buf = (uchar*)malloc(lenNikonLensData);
                fread(table_buf, lenNikonLensData, 1, ifp);
                if ((NikonLensDataVersion < 201) && lenNikonLensData)
                  {
                    processNikonLensData(table_buf, lenNikonLensData);
                    free(table_buf);
                    lenNikonLensData = 0;
                  }
              }
          }

        else if (tag == 0xa7)					// shutter count
          {
            NikonKey = fgetc(ifp) ^ fgetc(ifp) ^ fgetc(ifp) ^ fgetc(ifp);
            if ((NikonLensDataVersion > 200) && lenNikonLensData)
            {
              if (custom_serial)
              {
                ci = xlat[0][custom_serial];
              }
              else
              {
                ci = xlat[0][serial & 0xff];
              }
              cj = xlat[1][NikonKey];
              ck = 0x60;
              for (i = 0; i < lenNikonLensData; i++)
                table_buf[i] ^= (cj += ci * ck++);
              processNikonLensData(table_buf, lenNikonLensData);
              lenNikonLensData = 0;
              free(table_buf);
            }
          }
        else if (tag == 0x00a8)		// contains flash data
          {
          	for (i = 0; i < 4; i++)
              {
                NikonFlashInfoVersion = NikonFlashInfoVersion * 10 + fgetc(ifp) - '0';
              }
          }

        else if (tag == 37 && (!iso_speed || iso_speed == 65535))
          {
            unsigned char cc;
            fread(&cc, 1, 1, ifp);
            iso_speed = (int)(100.0 * powf64(2.0, (double)(cc) / 12.0 - 5.0));
            break;
          }
      }

    else if (!strncmp(make, "OLYMPUS", 7))
      {
        int SubDirOffsetValid =
              strncmp (model, "E-300", 5) &&
              strncmp (model, "E-330", 5) &&
              strncmp (model, "E-400", 5) &&
              strncmp (model, "E-500", 5) &&
              strncmp (model, "E-1", 3);

        if ((tag == 0x2010) || (tag == 0x2020))
          {
            fseek(ifp, save - 4, SEEK_SET);
            fseek(ifp, base + get4(), SEEK_SET);
            parse_makernote_0xc634(base, tag, dng_writer);
          }
        if (!SubDirOffsetValid &&
            ((len > 4) ||
             ( ((type == 3) || (type == 8)) && (len > 2))  ||
             ( ((type == 4) || (type == 9)) && (len > 1))  || (type == 5) || (type > 9)))
        goto skip_Oly_broken_tags;

        switch (tag) {
        case 0x0207:
        case 0x20100100:
          {
            uchar sOlyID[8];
            unsigned long long OlyID;
            fread (sOlyID, MIN(len,7), 1, ifp);
	    sOlyID[7] = 0;
            OlyID = sOlyID[0];
            i = 1;
            while (i < 7 && sOlyID[i])
              {
                OlyID = OlyID << 8 | sOlyID[i];
                i++;
              }
            setOlympusBodyFeatures(OlyID);
          }
          break;
        case 0x1002:
          imgdata.lens.makernotes.CurAp = powf64(2.0f, getreal(type)/2);
          break;
        case 0x20100102:
            stmread(imgdata.shootinginfo.InternalBodySerial, len, ifp);
        break;
        case 0x20100201:
          imgdata.lens.makernotes.LensID =
            (unsigned long long)fgetc(ifp)<<16 |
            (unsigned long long)(fgetc(ifp), fgetc(ifp))<<8 |
            (unsigned long long)fgetc(ifp);
          imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_FT;
          imgdata.lens.makernotes.LensFormat = LIBRAW_FORMAT_FT;
          if (((imgdata.lens.makernotes.LensID < 0x20000) ||
               (imgdata.lens.makernotes.LensID > 0x4ffff)) &&
              (imgdata.lens.makernotes.LensID & 0x10))
            {
              imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_mFT;
            }
          break;
        case 0x20100202:
          if ((!imgdata.lens.LensSerial[0]))
              stmread(imgdata.lens.LensSerial, len, ifp);
          break;
        case 0x20100203:
          stmread(imgdata.lens.makernotes.Lens,len, ifp);
          break;
        case 0x20100205:
          imgdata.lens.makernotes.MaxAp4MinFocal = powf64(sqrt(2.0f), get2() / 256.0f);
          break;
        case 0x20100206:
          imgdata.lens.makernotes.MaxAp4MaxFocal = powf64(sqrt(2.0f), get2() / 256.0f);
          break;
        case 0x20100207:
          imgdata.lens.makernotes.MinFocal = (float)get2();
          break;
        case 0x20100208:
          imgdata.lens.makernotes.MaxFocal = (float)get2();
          if (imgdata.lens.makernotes.MaxFocal > 1000.0f)
            imgdata.lens.makernotes.MaxFocal = imgdata.lens.makernotes.MinFocal;
          break;
        case 0x2010020a:
          imgdata.lens.makernotes.MaxAp4CurFocal = powf64(sqrt(2.0f), get2() / 256.0f);
          break;
        case 0x20100301:
          imgdata.lens.makernotes.TeleconverterID = fgetc(ifp) << 8;
          fgetc(ifp);
          imgdata.lens.makernotes.TeleconverterID =
            imgdata.lens.makernotes.TeleconverterID | fgetc(ifp);
          break;
        case 0x20100303:
          stmread(imgdata.lens.makernotes.Teleconverter, len, ifp);
          break;
        case 0x20100403:
          stmread(imgdata.lens.makernotes.Attachment,len, ifp);
          break;
        case 0x20200401:
	      imgdata.other.FlashEC = getreal(type);
	      break;
        }
        skip_Oly_broken_tags:;
      }

    else if (!strncmp(make, "PENTAX", 6) ||
             !strncmp(model, "PENTAX", 6) ||
             (!strncmp(make, "SAMSUNG", 7) && (dng_writer == CameraDNG)))
      {
        if (tag == 0x0005)
          {
            unique_id = get4();
            setPentaxBodyFeatures(unique_id);
          }
        else if (tag == 0x0013)
          {
            imgdata.lens.makernotes.CurAp = (float)get2()/10.0f;
          }
        else if (tag == 0x0014)
          {
            PentaxISO(get2());
          }
        else if (tag == 0x001d)
          {
            imgdata.lens.makernotes.CurFocal = (float)get4()/100.0f;
          }
        else if (tag == 0x003f)
          {
            imgdata.lens.makernotes.LensID = fgetc(ifp) << 8 | fgetc(ifp);
          }
        else if (tag == 0x004d)
          {
            if (type == 9) imgdata.other.FlashEC = getreal(type) / 256.0f;
            else imgdata.other.FlashEC = (float) ((signed short) fgetc(ifp)) / 6.0f;
          }
         else if (tag == 0x007e)
           {
             imgdata.color.linear_max[0] =
             imgdata.color.linear_max[1] =
             imgdata.color.linear_max[2] =
             imgdata.color.linear_max[3] = (long)(-1) * get4();
           }
        else if (tag == 0x0207)
          {
	    if(len < 65535) // Safety belt
               PentaxLensInfo(imgdata.lens.makernotes.CamID, len);
          }
        else if (tag == 0x020d)
        {
                FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Daylight][c ^ (c >> 1)] = get2();
        }
        else if (tag == 0x020e)
        {
                FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Shade][c ^ (c >> 1)] = get2();
        }
        else if (tag == 0x020f)
        {
                FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Cloudy][c ^ (c >> 1)] = get2();
        }
        else if (tag == 0x0210)
        {
                FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Tungsten][c ^ (c >> 1)] = get2();
        }
        else if (tag == 0x0211)
        {
                FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_D][c ^ (c >> 1)] = get2();
        }
        else if (tag == 0x0212)
        {
                FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_N][c ^ (c >> 1)] = get2();
        }
        else if (tag == 0x0213)
        {
                FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_W][c ^ (c >> 1)] = get2();
        }
        else if (tag == 0x0214)
        {
                FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Flash][c ^ (c >> 1)] = get2();
        }
        else if (tag == 0x0221)
        {
	  int nWB = get2();
	  if(nWB<=sizeof(imgdata.color.WBCT_Coeffs)/sizeof(imgdata.color.WBCT_Coeffs[0]))
	     for (int i = 0; i < nWB; i++)
	       {
		 imgdata.color.WBCT_Coeffs[i][0] = (unsigned)0xcfc6 - get2();
		 fseek(ifp, 2, SEEK_CUR);
		 imgdata.color.WBCT_Coeffs[i][1] = get2();
		 imgdata.color.WBCT_Coeffs[i][2] = imgdata.color.WBCT_Coeffs[i][4] = 0x2000;
		 imgdata.color.WBCT_Coeffs[i][3] = get2();
        	}
        }
        else if (tag == 0x0215)
        {
          fseek (ifp, 16, SEEK_CUR);
          sprintf(imgdata.shootinginfo.InternalBodySerial, "%d", get4());
        }
        else if (tag == 0x0229)
        {
          stmread(imgdata.shootinginfo.BodySerial, len, ifp);
        }
        else if (tag == 0x022d)
        {
	  fseek (ifp,2,SEEK_CUR);
          FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Daylight][c ^ (c >> 1)] = get2();
	  getc(ifp);
          FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Shade][c ^ (c >> 1)] = get2();
	  getc(ifp);
          FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Cloudy][c ^ (c >> 1)] = get2();
	  getc(ifp);
          FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Tungsten][c ^ (c >> 1)] = get2();
	  getc(ifp);
          FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_D][c ^ (c >> 1)] = get2();
	  getc(ifp);
          FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_N][c ^ (c >> 1)] = get2();
	  getc(ifp);
          FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_W][c ^ (c >> 1)] = get2();
	  getc(ifp);
          FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_Flash][c ^ (c >> 1)] = get2();
	  getc(ifp);
          FORC4 imgdata.color.WB_Coeffs[LIBRAW_WBI_FL_L][c ^ (c >> 1)] = get2();
        }
        else if (tag == 0x0239)		// Q-series lens info (LensInfoQ)
          {
            char LensInfo [20];
            fseek (ifp, 12, SEEK_CUR);
            stread(imgdata.lens.makernotes.Lens, 30, ifp);
            strcat(imgdata.lens.makernotes.Lens, " ");
            stread(LensInfo, 20, ifp);
            strcat(imgdata.lens.makernotes.Lens, LensInfo);
          }
      }

    else if (!strncmp(make, "SAMSUNG", 7) &&
             (dng_writer == AdobeDNG))
      {
        if (tag == 0x0002)
          {
            if(get4() == 0x2000)
              {
                imgdata.lens.makernotes.CameraMount = LIBRAW_MOUNT_Samsung_NX;
              }
            else if (!strncmp(model, "NX mini", 7))
              {
                imgdata.lens.makernotes.CameraMount = LIBRAW_MOUNT_Samsung_NX_M;
              }
            else
              {
                imgdata.lens.makernotes.CameraMount = LIBRAW_MOUNT_FixedLens;
                imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_FixedLens;
              }
          }
        else if (tag == 0x0003)
          {
            imgdata.lens.makernotes.CamID = unique_id = get4();
          }
        else if (tag == 0xa003)
          {
            imgdata.lens.makernotes.LensID = get2();
            if (imgdata.lens.makernotes.LensID)
              imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_Samsung_NX;
          }
        else if (tag == 0xa005)
          {
            stmread(imgdata.lens.InternalLensSerial, len, ifp);
          }
        else if (tag == 0xa019)
          {
            imgdata.lens.makernotes.CurAp = getreal(type);
          }
        else if (tag == 0xa01a)
          {
            imgdata.lens.makernotes.FocalLengthIn35mmFormat = get4() / 10.0f;
            if (imgdata.lens.makernotes.FocalLengthIn35mmFormat < 10.0f)
              imgdata.lens.makernotes.FocalLengthIn35mmFormat *= 10.0f;
          }
      }

    else if (!strncasecmp(make, "SONY", 4) ||
             !strncasecmp(make, "Konica", 6) ||
             !strncasecmp(make, "Minolta", 7) ||
             (!strncasecmp(make, "Hasselblad", 10) &&
              (!strncasecmp(model, "Stellar", 7) ||
               !strncasecmp(model, "Lunar", 5) ||
               !strncasecmp(model, "Lusso", 5) ||
               !strncasecmp(model, "HV",2))))
      {
        ushort lid;

        if (tag == 0xb001)			// Sony ModelID
          {
            unique_id = get2();
            setSonyBodyFeatures(unique_id);
            if (table_buf_0x9050_present)
              {
                process_Sony_0x9050(table_buf_0x9050, unique_id);
                free (table_buf_0x9050);
                table_buf_0x9050_present = 0;
              }
            if (table_buf_0x940c_present)
              {
                if (imgdata.lens.makernotes.CameraMount == LIBRAW_MOUNT_Sony_E)
                  {
                    process_Sony_0x940c(table_buf_0x940c);
                  }
                free (table_buf_0x940c);
                table_buf_0x940c_present = 0;
              }
          }
        else if ((tag == 0x0010) &&					// CameraInfo
                 strncasecmp(model, "DSLR-A100", 9) &&
                 strncasecmp(model, "NEX-5C", 6) &&
                 !strncasecmp(make, "SONY", 4) &&
                 ((len == 368) ||			// a700
                  (len == 5478) ||		// a850, a900
                  (len == 5506) ||		// a200, a300, a350
                  (len == 6118) ||		// a230, a290, a330, a380, a390

                  // a450, a500, a550, a560, a580
                  // a33, a35, a55
                  // NEX3, NEX5, NEX5C, NEXC3, VG10E
                  (len == 15360))
                 )
          {
            table_buf = (uchar*)malloc(len);
            fread(table_buf, len, 1, ifp);
            if (memcmp(table_buf, "\xff\xff\xff\xff\xff\xff\xff\xff", 8) &&
                memcmp(table_buf, "\x00\x00\x00\x00\x00\x00\x00\x00", 8))
              {
                switch (len) {
                case 368:
                case 5478:
                  // a700, a850, a900: CameraInfo
                  if (saneSonyCameraInfo(table_buf[0], table_buf[3], table_buf[2], table_buf[5], table_buf[4], table_buf[7]))
                    {
                      if (table_buf[0] | table_buf[3])
                        imgdata.lens.makernotes.MinFocal =
                          bcd2dec(table_buf[0]) * 100 + bcd2dec(table_buf[3]);
                      if (table_buf[2] | table_buf[5])
                        imgdata.lens.makernotes.MaxFocal =
                          bcd2dec(table_buf[2]) * 100 + bcd2dec(table_buf[5]);
                      if (table_buf[4])
                        imgdata.lens.makernotes.MaxAp4MinFocal = bcd2dec(table_buf[4]) / 10.0f;
                      if (table_buf[4])
                        imgdata.lens.makernotes.MaxAp4MaxFocal = bcd2dec(table_buf[7]) / 10.0f;
                      parseSonyLensFeatures(table_buf[1], table_buf[6]);
                    }
                  break;
                default:
                  // CameraInfo2 & 3
                  if (saneSonyCameraInfo(table_buf[1], table_buf[2], table_buf[3], table_buf[4], table_buf[5], table_buf[6]))
                    {
                      if (table_buf[1] | table_buf[2])
                        imgdata.lens.makernotes.MinFocal =
                          bcd2dec(table_buf[1]) * 100 + bcd2dec(table_buf[2]);
                      if (table_buf[3] | table_buf[4])
                        imgdata.lens.makernotes.MaxFocal =
                          bcd2dec(table_buf[3]) * 100 + bcd2dec(table_buf[4]);
                      if (table_buf[5])
                        imgdata.lens.makernotes.MaxAp4MinFocal = bcd2dec(table_buf[5]) / 10.0f;
                      if (table_buf[6])
                        imgdata.lens.makernotes.MaxAp4MaxFocal = bcd2dec(table_buf[6]) / 10.0f;
                      parseSonyLensFeatures(table_buf[0], table_buf[7]);
                    }
                }
              }
            free(table_buf);
          }

		else if (tag == 0x0104)
		  {
		    imgdata.other.FlashEC = getreal(type);
		  }

        else if (tag == 0x0105)					// Teleconverter
          {
            imgdata.lens.makernotes.TeleconverterID = get2();
          }

        else if (tag == 0x0114 && len < 65535)					// CameraSettings
          {
            table_buf = (uchar*)malloc(len);
            fread(table_buf, len, 1, ifp);
            switch (len) {
            case 280:
            case 364:
            case 332:
              // CameraSettings and CameraSettings2 are big endian
              if (table_buf[2] | table_buf[3])
                {
                  lid = (((ushort)table_buf[2])<<8) |
                    ((ushort)table_buf[3]);
                  imgdata.lens.makernotes.CurAp =
                    powf64(2.0f, ((float)lid/8.0f-1.0f)/2.0f);
                }
              break;
            case 1536:
            case 2048:
              // CameraSettings3 are little endian
              parseSonyLensType2(table_buf[1016], table_buf[1015]);
              if (imgdata.lens.makernotes.LensMount != LIBRAW_MOUNT_Canon_EF)
                {
                  switch (table_buf[153]) {
                  case 16: imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_Minolta_A; break;
                  case 17: imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_Sony_E; break;
                  }
                }
              break;
            }
            free(table_buf);
          }

        else if (tag == 0x9050 && len < 256000)		// little endian
          {
            table_buf_0x9050 = (uchar*)malloc(len);
            table_buf_0x9050_present = 1;
            fread(table_buf_0x9050, len, 1, ifp);

            if (imgdata.lens.makernotes.CamID)
              {
                process_Sony_0x9050(table_buf_0x9050, imgdata.lens.makernotes.CamID);
                free (table_buf_0x9050);
                table_buf_0x9050_present = 0;
              }
          }

        else if (tag == 0x940c && len < 256000)
          {
            table_buf_0x940c = (uchar*)malloc(len);
            table_buf_0x940c_present = 1;
            fread(table_buf_0x940c, len, 1, ifp);
            if ((imgdata.lens.makernotes.CamID) &&
                (imgdata.lens.makernotes.CameraMount == LIBRAW_MOUNT_Sony_E))
              {
                process_Sony_0x940c(table_buf_0x940c);
                free(table_buf_0x940c);
                table_buf_0x940c_present = 0;
              }
          }

        else if (((tag == 0xb027) || (tag == 0x010c)) && (imgdata.lens.makernotes.LensID == -1))
          {
            imgdata.lens.makernotes.LensID = get4();
            if ((imgdata.lens.makernotes.LensID > 0x4900) &&
                (imgdata.lens.makernotes.LensID <= 0x5900))
            {
              imgdata.lens.makernotes.AdapterID = 0x4900;
              imgdata.lens.makernotes.LensID -= imgdata.lens.makernotes.AdapterID;
              imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_Sigma_X3F;
              strcpy(imgdata.lens.makernotes.Adapter, "MC-11");
            }

            else if ((imgdata.lens.makernotes.LensID > 0xEF00) &&
                (imgdata.lens.makernotes.LensID < 0xFFFF)      &&
                (imgdata.lens.makernotes.LensID != 0xFF00))
            {
              imgdata.lens.makernotes.AdapterID = 0xEF00;
              imgdata.lens.makernotes.LensID -= imgdata.lens.makernotes.AdapterID;
              imgdata.lens.makernotes.LensMount = LIBRAW_MOUNT_Canon_EF;
            }
            if (tag == 0x010c) imgdata.lens.makernotes.CameraMount = LIBRAW_MOUNT_Minolta_A;
          }

        else if (tag == 0xb02a && len < 256000)	// Sony LensSpec
          {
            table_buf = (uchar*)malloc(len);
            fread(table_buf, len, 1, ifp);
            if (saneSonyCameraInfo(table_buf[1], table_buf[2], table_buf[3], table_buf[4], table_buf[5], table_buf[6]))
              {
                if (table_buf[1] | table_buf[2])
                  imgdata.lens.makernotes.MinFocal =
                    bcd2dec(table_buf[1]) * 100 + bcd2dec(table_buf[2]);
                if (table_buf[3] | table_buf[4])
                  imgdata.lens.makernotes.MaxFocal =
                    bcd2dec(table_buf[3]) * 100 + bcd2dec(table_buf[4]);
                if (table_buf[5])
                  imgdata.lens.makernotes.MaxAp4MinFocal = bcd2dec(table_buf[5]) / 10.0f;
                if (table_buf[6])
                  imgdata.lens.makernotes.MaxAp4MaxFocal = bcd2dec(table_buf[6]) / 10.0f;
                parseSonyLensFeatures(table_buf[0], table_buf[7]);
              }
            free(table_buf);
          }
      }
  next:
    fseek (ifp, save, SEEK_SET);
  }
 quit:
  order = sorder;
}