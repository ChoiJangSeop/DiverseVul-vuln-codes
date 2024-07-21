void CLASS nikon_coolscan_load_raw()
{
  int bufsize = width*3*tiff_bps/8;
  if(tiff_bps <= 8)
    gamma_curve(1.0/imgdata.params.coolscan_nef_gamma,0.,1,255);
  else
    gamma_curve(1.0/imgdata.params.coolscan_nef_gamma,0.,1,65535);
  fseek (ifp, data_offset, SEEK_SET);
  unsigned char *buf = (unsigned char*)malloc(bufsize);
  unsigned short *ubuf = (unsigned short *)buf;
  for(int row = 0; row < raw_height; row++)
    {
      int red = fread (buf, 1, bufsize, ifp);
      unsigned short (*ip)[4] = (unsigned short (*)[4]) image + row*width;
      if(tiff_bps <= 8)
        for(int col=0; col<width;col++)
          {
            ip[col][0] = curve[buf[col*3]];
            ip[col][1] = curve[buf[col*3+1]];
            ip[col][2] = curve[buf[col*3+2]];
            ip[col][3]=0;
          }
      else
        for(int col=0; col<width;col++)
          {
            ip[col][0] = curve[ubuf[col*3]];
            ip[col][1] = curve[ubuf[col*3+1]];
            ip[col][2] = curve[ubuf[col*3+2]];
            ip[col][3]=0;
          }
    }
  free(buf);
}