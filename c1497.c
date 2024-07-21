void CLASS apply_tiff()
{
  int max_samp=0, raw=-1, thm=-1, i;
  struct jhead jh;

  thumb_misc = 16;
  if (thumb_offset) {
    fseek (ifp, thumb_offset, SEEK_SET);
    if (ljpeg_start (&jh, 1)) {
      thumb_misc   = jh.bits;
      thumb_width  = jh.wide;
      thumb_height = jh.high;
    }
  }
  for (i=0; i < tiff_nifds; i++) {
    if (max_samp < tiff_ifd[i].samples)
	max_samp = tiff_ifd[i].samples;
    if (max_samp > 3) max_samp = 3;
    if ((tiff_ifd[i].comp != 6 || tiff_ifd[i].samples != 3) &&
	(tiff_ifd[i].t_width | tiff_ifd[i].t_height) < 0x10000 &&
	tiff_ifd[i].t_width*tiff_ifd[i].t_height > raw_width*raw_height) {
      raw_width     = tiff_ifd[i].t_width;
      raw_height    = tiff_ifd[i].t_height;
      tiff_bps      = tiff_ifd[i].bps;
      tiff_compress = tiff_ifd[i].comp;
      data_offset   = tiff_ifd[i].offset;
      tiff_flip     = tiff_ifd[i].t_flip;
      tiff_samples  = tiff_ifd[i].samples;
      tile_width    = tiff_ifd[i].t_tile_width;
      tile_length   = tiff_ifd[i].t_tile_length;
#ifdef LIBRAW_LIBRARY_BUILD
      data_size     = tile_length < INT_MAX && tile_length>0 ? tiff_ifd[i].tile_maxbytes: tiff_ifd[i].bytes;
#endif
      raw = i;
    }
  }
  if (!tile_width ) tile_width  = INT_MAX;
  if (!tile_length) tile_length = INT_MAX;
  for (i=tiff_nifds; i--; )
    if (tiff_ifd[i].t_flip) tiff_flip = tiff_ifd[i].t_flip;
  if (raw >= 0 && !load_raw)
    switch (tiff_compress) {
      case 32767:
	if (tiff_ifd[raw].bytes == raw_width*raw_height) {
	  tiff_bps = 12;
	  load_raw = &CLASS sony_arw2_load_raw;			break;
	}
	if (tiff_ifd[raw].bytes*8 != raw_width*raw_height*tiff_bps) {
	  raw_height += 8;
	  load_raw = &CLASS sony_arw_load_raw;			break;
	}
	load_flags = 79;
      case 32769:
	load_flags++;
      case 32770:
      case 32773: goto slr;
      case 0:  case 1:
	if (tiff_ifd[raw].bytes*5 == raw_width*raw_height*8) {
	  load_flags = 81;
	  tiff_bps = 12;
	} slr:
	switch (tiff_bps) {
	  case  8: load_raw = &CLASS eight_bit_load_raw;	break;
	  case 12: if (tiff_ifd[raw].phint == 2)
		     load_flags = 6;
		   load_raw = &CLASS packed_load_raw;		break;
	  case 14: load_flags = 0;
	  case 16: load_raw = &CLASS unpacked_load_raw;		break;
	}
	break;
      case 6:  case 7:  case 99:
	load_raw = &CLASS lossless_jpeg_load_raw;		break;
      case 262:
	load_raw = &CLASS kodak_262_load_raw;			break;
      case 34713:
	if ((raw_width+9)/10*16*raw_height == tiff_ifd[raw].bytes) {
	  load_raw = &CLASS packed_load_raw;
	  load_flags = 1;
	} else if (raw_width*raw_height*2 == tiff_ifd[raw].bytes) {
	  load_raw = &CLASS unpacked_load_raw;
	  load_flags = 4;
	  order = 0x4d4d;
	} else
	  load_raw = &CLASS nikon_load_raw;			break;
      case 34892:
	load_raw = &CLASS lossy_dng_load_raw;			break;
      case 65535:
	load_raw = &CLASS pentax_load_raw;			break;
      case 65000:
	switch (tiff_ifd[raw].phint) {
	  case 2: load_raw = &CLASS kodak_rgb_load_raw;   filters = 0;  break;
	  case 6: load_raw = &CLASS kodak_ycbcr_load_raw; filters = 0;  break;
	  case 32803: load_raw = &CLASS kodak_65000_load_raw;
	}
      case 32867: break;
      default: is_raw = 0;
    }
  if (!dng_version)
    if ( (tiff_samples == 3 && tiff_ifd[raw].bytes &&
	  tiff_bps != 14 && tiff_bps != 2048 && 
	  tiff_compress != 32769 && tiff_compress != 32770)
      || (tiff_bps == 8 && !strstr(make,"KODAK") && !strstr(make,"Kodak") &&
	  !strstr(model2,"DEBUG RAW")))
      is_raw = 0;
  for (i=0; i < tiff_nifds; i++)
    if (i != raw && tiff_ifd[i].samples == max_samp &&
	tiff_ifd[i].t_width * tiff_ifd[i].t_height / SQR(tiff_ifd[i].bps+1) >
	      thumb_width *       thumb_height / SQR(thumb_misc+1)
	&& tiff_ifd[i].comp != 34892) {
      thumb_width  = tiff_ifd[i].t_width;
      thumb_height = tiff_ifd[i].t_height;
      thumb_offset = tiff_ifd[i].offset;
      thumb_length = tiff_ifd[i].bytes;
      thumb_misc   = tiff_ifd[i].bps;
      thm = i;
    }
  if (thm >= 0) {
    thumb_misc |= tiff_ifd[thm].samples << 5;
    switch (tiff_ifd[thm].comp) {
      case 0:
	write_thumb = &CLASS layer_thumb;
	break;
      case 1:
	if (tiff_ifd[thm].bps <= 8)
	  write_thumb = &CLASS ppm_thumb;
	else if (!strcmp(make,"Imacon"))
	  write_thumb = &CLASS ppm16_thumb;
	else
	  thumb_load_raw = &CLASS kodak_thumb_load_raw;
	break;
      case 65000:
	thumb_load_raw = tiff_ifd[thm].phint == 6 ?
		&CLASS kodak_ycbcr_load_raw : &CLASS kodak_rgb_load_raw;
    }
  }
}