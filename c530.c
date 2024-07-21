hb_buffer_add_glyph( HB_Buffer buffer,
		      HB_UInt   glyph_index,
		      HB_UInt   properties,
		      HB_UInt   cluster )
{
  HB_Error error;
  HB_GlyphItem glyph;
  
  error = hb_buffer_ensure( buffer, buffer->in_length + 1 );
  if ( error )
    return error;

  glyph = &buffer->in_string[buffer->in_length];
  glyph->gindex = glyph_index;
  glyph->properties = properties;
  glyph->cluster = cluster;
  glyph->component = 0;
  glyph->ligID = 0;
  glyph->gproperty = HB_GLYPH_PROPERTY_UNKNOWN;
  
  buffer->in_length++;

  return HB_Err_Ok;
}