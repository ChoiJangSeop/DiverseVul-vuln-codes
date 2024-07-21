_hb_buffer_replace_glyph( HB_Buffer buffer,
			  HB_UInt   glyph_index )
{
  if ( !buffer->separate_out )
    {
      buffer->out_string[buffer->out_pos].gindex = glyph_index;

      buffer->in_pos++;
      buffer->out_pos++;
      buffer->out_length = buffer->out_pos;
    }
  else
    {
      return _hb_buffer_add_output_glyph( buffer, glyph_index, 0xFFFF, 0xFFFF );
    }

  return HB_Err_Ok;
}