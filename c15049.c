void LibRaw::parse_x3f()
{
  x3f_t *x3f = x3f_new_from_file(libraw_internal_data.internal_data.input);
  if(!x3f)
      return;
  _x3f_data = x3f;

  x3f_header_t *H = NULL;
  x3f_directory_section_t *DS = NULL;

  H = &x3f->header;
  // Parse RAW size from RAW section
  x3f_directory_entry_t *DE = x3f_get_raw(x3f);
  if(!DE) return;
  imgdata.sizes.flip = H->rotation;
  x3f_directory_entry_header_t *DEH = &DE->header;
  x3f_image_data_t *ID = &DEH->data_subsection.image_data;
  imgdata.sizes.raw_width = ID->columns;
  imgdata.sizes.raw_height = ID->rows;
  // Parse other params from property section
  DE = x3f_get_prop(x3f);
  if((x3f_load_data(x3f,DE) == X3F_OK))
  {
	  // Parse property list
	  DEH = &DE->header;
	  x3f_property_list_t *PL = &DEH->data_subsection.property_list;
	  if (PL->property_table.size != 0) {
		  int i;
		  x3f_property_t *P = PL->property_table.element;
		  for (i=0; i<PL->num_properties; i++) {
			  char name[100], value[100];
			  utf2char(P[i].name,name);
			  utf2char(P[i].value,value);
			  if (!strcmp (name, "ISO"))
				  imgdata.other.iso_speed = atoi(value);
			  if (!strcmp (name, "CAMMANUF"))
				  strcpy (imgdata.idata.make, value);
			  if (!strcmp (name, "CAMMODEL"))
				  strcpy (imgdata.idata.model, value);
			  if (!strcmp (name, "CAMSERIAL"))
				  strcpy (imgdata.shootinginfo.BodySerial, value);
			  if (!strcmp (name, "WB_DESC"))
				  strcpy (imgdata.color.model2, value);
			  if (!strcmp (name, "TIME"))
				  imgdata.other.timestamp = atoi(value);
			  if (!strcmp (name, "SHUTTER"))
				  imgdata.other.shutter = atof(value);
			  if (!strcmp (name, "APERTURE"))
				  imgdata.other.aperture = atof(value);
			  if (!strcmp (name, "FLENGTH"))
				  imgdata.other.focal_len = atof(value);
				if (!strcmp (name, "FLEQ35MM"))
				  imgdata.lens.makernotes.FocalLengthIn35mmFormat = atof(value);
				if (!strcmp (name, "LENSARANGE"))
				{
				  char *sp;
				  imgdata.lens.makernotes.MaxAp4CurFocal = imgdata.lens.makernotes.MinAp4CurFocal = atof(value);
				  sp = strrchr (value, ' ');
				  if (sp)
				    {
				      imgdata.lens.makernotes.MinAp4CurFocal = atof(sp);
				      if (imgdata.lens.makernotes.MaxAp4CurFocal > imgdata.lens.makernotes.MinAp4CurFocal)
				        my_swap (float, imgdata.lens.makernotes.MaxAp4CurFocal, imgdata.lens.makernotes.MinAp4CurFocal);
				    }
				}
				if (!strcmp (name, "LENSFRANGE"))
				{
					char *sp;
					imgdata.lens.makernotes.MinFocal = imgdata.lens.makernotes.MaxFocal = atof(value);
					sp = strrchr (value, ' ');
					if (sp)
						{
							imgdata.lens.makernotes.MaxFocal = atof(sp);
							if ((imgdata.lens.makernotes.MaxFocal + 0.17f) < imgdata.lens.makernotes.MinFocal)
								my_swap (float, imgdata.lens.makernotes.MaxFocal, imgdata.lens.makernotes.MinFocal);
						}
				}
				if (!strcmp (name, "LENSMODEL"))
				{
					char *sp;
                                        imgdata.lens.makernotes.LensID = strtol (value, &sp, 16); // atoi(value);
					if (imgdata.lens.makernotes.LensID)
					 imgdata.lens.makernotes.LensMount = Sigma_X3F;
				}
		  }
		  imgdata.idata.raw_count=1;
		  load_raw = &LibRaw::x3f_load_raw;
		  imgdata.sizes.raw_pitch = imgdata.sizes.raw_width*6;
		  imgdata.idata.is_foveon = 1;
		  libraw_internal_data.internal_output_params.raw_color=1; // Force adobe coeff
		  imgdata.color.maximum=0x3fff; // To be reset by color table
		  libraw_internal_data.unpacker_data.order = 0x4949;
	  }
  }
  else
  {
	  // No property list
	  if(imgdata.sizes.raw_width == 5888 ||imgdata.sizes.raw_width == 2944 
		  || imgdata.sizes.raw_width == 6656 ||imgdata.sizes.raw_width == 3328 	  
		  || imgdata.sizes.raw_width == 5504 ||imgdata.sizes.raw_width == 2752 	  
		  ) // Quattro
	  {
		  imgdata.idata.raw_count=1;
		  load_raw = &LibRaw::x3f_load_raw;
		  imgdata.sizes.raw_pitch = imgdata.sizes.raw_width*6;
		  imgdata.idata.is_foveon = 1;
		  libraw_internal_data.internal_output_params.raw_color=1; // Force adobe coeff
		  libraw_internal_data.unpacker_data.order = 0x4949;
		  strcpy (imgdata.idata.make, "SIGMA");
#if 1
		  // Try to find model number in first 2048 bytes;
		  int pos = libraw_internal_data.internal_data.input->tell();
		  libraw_internal_data.internal_data.input->seek(0,SEEK_SET);
		  unsigned char buf[2048];
		  libraw_internal_data.internal_data.input->read(buf,2048,1);
		  libraw_internal_data.internal_data.input->seek(pos,SEEK_SET);
		  unsigned char *fnd=(unsigned char*)lr_memmem(buf,2048,"SIGMA dp",8);
		  unsigned char *fndsd=(unsigned char*)lr_memmem(buf,2048,"sd Quatt",8);
		  if(fnd)
		  {
			  unsigned char *nm = fnd+8;
			  snprintf(imgdata.idata.model,64,"dp%c Quattro",*nm<='9' && *nm >='0' ? *nm: '2');
		  }
		  else if(fndsd)
		  {
			  snprintf(imgdata.idata.model,64,"%s",fndsd);
		  }
		  else
#endif
		  if(imgdata.sizes.raw_width == 6656 ||imgdata.sizes.raw_width == 3328 )
			strcpy (imgdata.idata.model, "sd Quattro H");
		  else
			strcpy (imgdata.idata.model, "dp2 Quattro");
	  }
	  //else
  }
  // Try to get thumbnail data
  LibRaw_thumbnail_formats format = LIBRAW_THUMBNAIL_UNKNOWN;
  if( (DE = x3f_get_thumb_jpeg(x3f)))
    {
      format = LIBRAW_THUMBNAIL_JPEG;
    }
  else if( (DE = x3f_get_thumb_plain(x3f)))
    {
      format = LIBRAW_THUMBNAIL_BITMAP;
    }
  if(DE)
    {
      x3f_directory_entry_header_t *DEH = &DE->header;
      x3f_image_data_t *ID = &DEH->data_subsection.image_data;
      imgdata.thumbnail.twidth = ID->columns;
      imgdata.thumbnail.theight = ID->rows;
      imgdata.thumbnail.tcolors = 3;
      imgdata.thumbnail.tformat = format;
      libraw_internal_data.internal_data.toffset = DE->input.offset;
      write_thumb = &LibRaw::x3f_thumb_loader;
    }
}