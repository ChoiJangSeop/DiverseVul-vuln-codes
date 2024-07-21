_hb_buffer_next_glyph ( HB_Buffer buffer )
{
  HB_Error  error;

  if ( buffer->separate_out )
    {
      error = hb_buffer_ensure( buffer, buffer->out_pos + 1 );
      if ( error )
	return error;

      buffer->out_string[buffer->out_pos] = buffer->in_string[buffer->in_pos];
    }

  buffer->in_pos++;
  buffer->out_pos++;
  buffer->out_length = buffer->out_pos;

  return HB_Err_Ok;
}