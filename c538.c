hb_buffer_clear( HB_Buffer buffer )
{
  buffer->in_length = 0;
  buffer->out_length = 0;
  buffer->in_pos = 0;
  buffer->out_pos = 0;
  buffer->out_string = buffer->in_string;
  buffer->separate_out = FALSE;
  buffer->max_ligID = 0;
}