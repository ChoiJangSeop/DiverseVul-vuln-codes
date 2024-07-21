hb_buffer_new( HB_Buffer *pbuffer )
{
  HB_Buffer buffer;
  HB_Error error;
  
  if ( ALLOC( buffer, sizeof( HB_BufferRec ) ) )
    return error;

  buffer->allocated = 0;
  buffer->in_string = NULL;
  buffer->alt_string = NULL;
  buffer->positions = NULL;

  hb_buffer_clear( buffer );

  *pbuffer = buffer;

  return HB_Err_Ok;
}