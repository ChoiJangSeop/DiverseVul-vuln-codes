load_image (gchar *filename)
{
  FILE *fd;
  char * name_buf;
  unsigned char buf[16];
  unsigned char c;
  CMap localColorMap;
  int grayScale;
  int useGlobalColormap;
  int bitPixel;
  int imageCount = 0;
  char version[4];
  gint32 image_ID = -1;

  fd = fopen (filename, "rb");
  if (!fd)
    {
      g_message ("GIF: can't open \"%s\"\n", filename);
      return -1;
    }

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    {
      name_buf = g_strdup_printf (_("Loading %s:"), filename);
      gimp_progress_init (name_buf);
      g_free (name_buf);
    }

  if (!ReadOK (fd, buf, 6))
    {
      g_message ("GIF: error reading magic number\n");
      return -1;
    }

  if (strncmp ((char *) buf, "GIF", 3) != 0)
    {
      g_message ("GIF: not a GIF file\n");
      return -1;
    }

  strncpy (version, (char *) buf + 3, 3);
  version[3] = '\0';

  if ((strcmp (version, "87a") != 0) && (strcmp (version, "89a") != 0))
    {
      g_message ("GIF: bad version number, not '87a' or '89a'\n");
      return -1;
    }

  if (!ReadOK (fd, buf, 7))
    {
      g_message ("GIF: failed to read screen descriptor\n");
      return -1;
    }

  GifScreen.Width = LM_to_uint (buf[0], buf[1]);
  GifScreen.Height = LM_to_uint (buf[2], buf[3]);
  GifScreen.BitPixel = 2 << (buf[4] & 0x07);
  GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
  GifScreen.Background = buf[5];
  GifScreen.AspectRatio = buf[6];

  if (BitSet (buf[4], LOCALCOLORMAP))
    {
      /* Global Colormap */
      if (ReadColorMap (fd, GifScreen.BitPixel, GifScreen.ColorMap, &GifScreen.GrayScale))
	{
	  g_message ("GIF: error reading global colormap\n");
	  return -1;
	}
    }

  if (GifScreen.AspectRatio != 0 && GifScreen.AspectRatio != 49)
    {
      g_message ("GIF: warning - non-square pixels\n");
    }


  highest_used_index = 0;
      

  for (;;)
    {
      if (!ReadOK (fd, &c, 1))
	{
	  g_message ("GIF: EOF / read error on image data\n");
	  return image_ID; /* will be -1 if failed on first image! */
	}

      if (c == ';')
	{
	  /* GIF terminator */
	  return image_ID;
	}

      if (c == '!')
	{
	  /* Extension */
	  if (!ReadOK (fd, &c, 1))
	    {
	      g_message ("GIF: EOF / read error on extension function code\n");
	      return image_ID; /* will be -1 if failed on first image! */
	    }
	  DoExtension (fd, c);
	  continue;
	}

      if (c != ',')
	{
	  /* Not a valid start character */
	  g_warning ("GIF: bogus character 0x%02x, ignoring\n", (int) c);
	  continue;
	}

      ++imageCount;

      if (!ReadOK (fd, buf, 9))
	{
	  g_message ("GIF: couldn't read left/top/width/height\n");
	  return image_ID; /* will be -1 if failed on first image! */
	}

      useGlobalColormap = !BitSet (buf[8], LOCALCOLORMAP);

      bitPixel = 1 << ((buf[8] & 0x07) + 1);

      if (!useGlobalColormap)
	{
	  if (ReadColorMap (fd, bitPixel, localColorMap, &grayScale))
	    {
	      g_message ("GIF: error reading local colormap\n");
	      return image_ID; /* will be -1 if failed on first image! */
	    }
	  image_ID = ReadImage (fd, filename, LM_to_uint (buf[4], buf[5]),
				LM_to_uint (buf[6], buf[7]),
				localColorMap, bitPixel,
				grayScale,
				BitSet (buf[8], INTERLACE), imageCount,
				(guint) LM_to_uint (buf[0], buf[1]),
				(guint) LM_to_uint (buf[2], buf[3]),
				GifScreen.Width,
				GifScreen.Height
				);
	}
      else
	{
	  image_ID = ReadImage (fd, filename, LM_to_uint (buf[4], buf[5]),
				LM_to_uint (buf[6], buf[7]),
				GifScreen.ColorMap, GifScreen.BitPixel,
				GifScreen.GrayScale,
				BitSet (buf[8], INTERLACE), imageCount,
				(guint) LM_to_uint (buf[0], buf[1]),
				(guint) LM_to_uint (buf[2], buf[3]),
				GifScreen.Width,
				GifScreen.Height
				);
	}

#ifdef FACEHUGGERS
      if (comment_parasite != NULL)
	{
	  gimp_image_parasite_attach (image_ID, comment_parasite);
	  gimp_parasite_free (comment_parasite);
	  comment_parasite = NULL;
	}
#endif

    }

  return image_ID;
}