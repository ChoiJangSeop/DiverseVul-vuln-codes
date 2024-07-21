_hb_buffer_clear_positions( HB_Buffer buffer )
{
  _hb_buffer_clear_output (buffer);

  if ( !buffer->positions )
    {
      HB_Error error;

      if ( ALLOC_ARRAY( buffer->positions, buffer->allocated, HB_PositionRec ) )
	return error;
    }

  memset (buffer->positions, 0, sizeof (buffer->positions[0]) * buffer->in_length);

  return HB_Err_Ok;
}