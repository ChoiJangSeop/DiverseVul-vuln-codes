_hb_buffer_clear_output( HB_Buffer buffer )
{
  buffer->out_length = 0;
  buffer->out_pos = 0;
  buffer->out_string = buffer->in_string;
  buffer->separate_out = FALSE;
}