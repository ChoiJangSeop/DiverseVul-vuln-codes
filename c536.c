hb_buffer_free( HB_Buffer buffer )
{
  FREE( buffer->in_string );
  FREE( buffer->alt_string );
  buffer->out_string = NULL;
  FREE( buffer->positions );
  FREE( buffer );
}