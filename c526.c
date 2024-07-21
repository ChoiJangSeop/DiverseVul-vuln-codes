_hb_buffer_add_output_glyph( HB_Buffer buffer,
			     HB_UInt   glyph_index,
			     HB_UShort component,
			     HB_UShort ligID )
{
  HB_UShort glyph_data =  glyph_index;

  return _hb_buffer_add_output_glyphs ( buffer, 1, 1,
					&glyph_data, component, ligID );
}