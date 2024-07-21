_hb_buffer_swap( HB_Buffer buffer )
{
  HB_GlyphItem tmp_string;
  int tmp_length;
  int tmp_pos;

  if ( buffer->separate_out )
    {
      tmp_string = buffer->in_string;
      buffer->in_string = buffer->out_string;
      buffer->out_string = tmp_string;
      buffer->alt_string = buffer->out_string;
    }

  tmp_length = buffer->in_length;
  buffer->in_length = buffer->out_length;
  buffer->out_length = tmp_length;

  tmp_pos = buffer->in_pos;
  buffer->in_pos = buffer->out_pos;
  buffer->out_pos = tmp_pos;
}