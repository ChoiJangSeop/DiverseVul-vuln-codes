png_read_chunk_header(png_structrp png_ptr)
{
   png_byte buf[8];
   png_uint_32 length;
   png_alloc_size_t limit = PNG_UINT_31_MAX;

#ifdef PNG_IO_STATE_SUPPORTED
   png_ptr->io_state = PNG_IO_READING | PNG_IO_CHUNK_HDR;
#endif

   /* Read the length and the chunk name.
    * This must be performed in a single I/O call.
    */
   png_read_data(png_ptr, buf, 8);
   length = png_get_uint_31(png_ptr, buf);

   /* Put the chunk name into png_ptr->chunk_name. */
   png_ptr->chunk_name = PNG_CHUNK_FROM_STRING(buf+4);

   png_debug2(0, "Reading %lx chunk, length = %lu",
       (unsigned long)png_ptr->chunk_name, (unsigned long)length);

   /* Reset the crc and run it over the chunk name. */
   png_reset_crc(png_ptr);
   png_calculate_crc(png_ptr, buf + 4, 4);

   /* Check to see if chunk name is valid. */
   png_check_chunk_name(png_ptr, png_ptr->chunk_name);

   /* Check for too-large chunk length */
   if (png_ptr->chunk_name != png_IDAT)
   {
# ifdef PNG_SET_USER_LIMITS_SUPPORTED
      if (png_ptr->user_chunk_malloc_max > 0 &&
          png_ptr->user_chunk_malloc_max < limit)
         limit = png_ptr->user_chunk_malloc_max;
# elif PNG_USER_CHUNK_MALLOC_MAX > 0
      if (PNG_USER_CHUNK_MALLOC_MAX < limit)
         limit = PNG_USER_CHUNK_MALLOC_MAX;
# endif
   }
   else
   {
      size_t row_factor =
         (png_ptr->width * png_ptr->channels * (png_ptr->bit_depth > 8? 2: 1)
          + 1 + (png_ptr->interlaced? 6: 0));
      if (png_ptr->height > PNG_UINT_32_MAX/row_factor)
         limit=PNG_UINT_31_MAX;
      else
         limit = png_ptr->height * row_factor;
      limit += 6 + 5*(limit/32566+1); /* zlib+deflate overhead */
      limit=limit < PNG_UINT_31_MAX? limit : PNG_UINT_31_MAX;
   }

   if (length > limit)
   {
      png_debug2(0," length = %lu, limit = %lu",
         (unsigned long)length,(unsigned long)limit);
      png_chunk_error(png_ptr, "chunk data is too large");
   }

#ifdef PNG_IO_STATE_SUPPORTED
   png_ptr->io_state = PNG_IO_READING | PNG_IO_CHUNK_DATA;
#endif

   return length;
}