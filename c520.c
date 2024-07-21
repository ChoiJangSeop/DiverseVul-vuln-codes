hb_buffer_ensure( HB_Buffer buffer,
		   HB_UInt   size )
{
  HB_UInt new_allocated = buffer->allocated;

  if (size > new_allocated)
    {
      HB_Error error;

      while (size > new_allocated)
	new_allocated += (new_allocated >> 1) + 8;

      if ( buffer->positions )
        {
	  if ( REALLOC_ARRAY( buffer->positions, new_allocated, HB_PositionRec ) )
	    return error;
	}

      if ( REALLOC_ARRAY( buffer->in_string, new_allocated, HB_GlyphItemRec ) )
	return error;

      if ( buffer->separate_out )
        {
	  if ( REALLOC_ARRAY( buffer->alt_string, new_allocated, HB_GlyphItemRec ) )
	    return error;

	  buffer->out_string = buffer->alt_string;
	}
      else
        {
	  buffer->out_string = buffer->in_string;

	  if ( buffer->alt_string )
	    {
	      if ( REALLOC_ARRAY( buffer->alt_string, new_allocated, HB_GlyphItemRec ) )
		return error;
	    }
	}

      buffer->allocated = new_allocated;
    }

  return HB_Err_Ok;
}