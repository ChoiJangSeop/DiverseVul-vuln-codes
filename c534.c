hb_buffer_duplicate_out_buffer( HB_Buffer buffer )
{
  if ( !buffer->alt_string )
    {
      HB_Error error;

      if ( ALLOC_ARRAY( buffer->alt_string, buffer->allocated, HB_GlyphItemRec ) )
	return error;
    }

  buffer->out_string = buffer->alt_string;
  memcpy( buffer->out_string, buffer->in_string, buffer->out_length * sizeof (buffer->out_string[0]) );
  buffer->separate_out = TRUE;

  return HB_Err_Ok;
}