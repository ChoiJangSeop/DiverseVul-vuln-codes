Magick_png_write_raw_profile(const ImageInfo *image_info,png_struct *ping,
   png_info *ping_info, unsigned char *profile_type, unsigned char
   *profile_description, unsigned char *profile_data, png_uint_32 length)
{
   png_textp
     text;

   register ssize_t
     i;

   unsigned char
     *sp;

   png_charp
     dp;

   png_uint_32
     allocated_length,
     description_length;

   unsigned char
     hex[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

   if (LocaleNCompare((char *) profile_type+1, "ng-chunk-",9) == 0)
      return;

   if (image_info->verbose)
     {
       (void) printf("writing raw profile: type=%s, length=%.20g\n",
         (char *) profile_type, (double) length);
     }

#if PNG_LIBPNG_VER >= 10400
   text=(png_textp) png_malloc(ping,(png_alloc_size_t) sizeof(png_text));
#else
   text=(png_textp) png_malloc(ping,(png_size_t) sizeof(png_text));
#endif
   description_length=(png_uint_32) strlen((const char *) profile_description);
   allocated_length=(png_uint_32) (length*2 + (length >> 5) + 20
      + description_length);
#if PNG_LIBPNG_VER >= 10400
   text[0].text=(png_charp) png_malloc(ping,
      (png_alloc_size_t) allocated_length);
   text[0].key=(png_charp) png_malloc(ping, (png_alloc_size_t) 80);
#else
   text[0].text=(png_charp) png_malloc(ping, (png_size_t) allocated_length);
   text[0].key=(png_charp) png_malloc(ping, (png_size_t) 80);
#endif
   text[0].key[0]='\0';
   (void) ConcatenateMagickString(text[0].key,
      "Raw profile type ",MagickPathExtent);
   (void) ConcatenateMagickString(text[0].key,(const char *) profile_type,62);
   sp=profile_data;
   dp=text[0].text;
   *dp++='\n';
   (void) CopyMagickString(dp,(const char *) profile_description,
     allocated_length);
   dp+=description_length;
   *dp++='\n';
   (void) FormatLocaleString(dp,allocated_length-
     (png_size_t) (dp-text[0].text),"%8lu ",(unsigned long) length);
   dp+=8;

   for (i=0; i < (ssize_t) length; i++)
   {
     if (i%36 == 0)
       *dp++='\n';
     *(dp++)=(char) hex[((*sp >> 4) & 0x0f)];
     *(dp++)=(char) hex[((*sp++ ) & 0x0f)];
   }

   *dp++='\n';
   *dp='\0';
   text[0].text_length=(png_size_t) (dp-text[0].text);
   text[0].compression=image_info->compression == NoCompression ||
     (image_info->compression == UndefinedCompression &&
     text[0].text_length < 128) ? -1 : 0;

   if (text[0].text_length <= allocated_length)
     png_set_text(ping,ping_info,text,1);

   png_free(ping,text[0].text);
   png_free(ping,text[0].key);
   png_free(ping,text);
}