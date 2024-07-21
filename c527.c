_hb_buffer_add_output_glyphs( HB_Buffer  buffer,
			      HB_UShort  num_in,
			      HB_UShort  num_out,
			      HB_UShort *glyph_data,
			      HB_UShort  component,
			      HB_UShort  ligID )
{
  HB_Error  error;
  HB_UShort i;
  HB_UInt properties;
  HB_UInt cluster;

  error = hb_buffer_ensure( buffer, buffer->out_pos + num_out );
  if ( error )
    return error;

  if ( !buffer->separate_out )
    {
      error = hb_buffer_duplicate_out_buffer( buffer );
      if ( error )
	return error;
    }

  properties = buffer->in_string[buffer->in_pos].properties;
  cluster = buffer->in_string[buffer->in_pos].cluster;
  if ( component == 0xFFFF )
    component = buffer->in_string[buffer->in_pos].component;
  if ( ligID == 0xFFFF )
    ligID = buffer->in_string[buffer->in_pos].ligID;

  for ( i = 0; i < num_out; i++ )
  {
    HB_GlyphItem item = &buffer->out_string[buffer->out_pos + i];

    item->gindex = glyph_data[i];
    item->properties = properties;
    item->cluster = cluster;
    item->component = component;
    item->ligID = ligID;
    item->gproperty = HB_GLYPH_PROPERTY_UNKNOWN;
  }

  buffer->in_pos  += num_in;
  buffer->out_pos += num_out;

  buffer->out_length = buffer->out_pos;

  return HB_Err_Ok;
}