_pango_ot_info_position    (const PangoOTInfo    *info,
			    const PangoOTRuleset *ruleset,
			    PangoOTBuffer        *buffer)
{
  unsigned int i;

  _hb_buffer_clear_positions (buffer->buffer);

  hb_ot_layout_set_direction (info->layout,
			      buffer->rtl);
  hb_ot_layout_set_hinting (info->layout,
			    buffer->font->is_hinted);
  hb_ot_layout_set_scale (info->layout,
			  info->face->size->metrics.x_scale,
			  info->face->size->metrics.y_scale);
  hb_ot_layout_set_ppem (info->layout,
			 info->face->size->metrics.x_ppem,
			 info->face->size->metrics.y_ppem);

  for (i = 0; i < ruleset->rules->len; i++)
    {
      PangoOTRule *rule = &g_array_index (ruleset->rules, PangoOTRule, i);
      hb_ot_layout_feature_mask_t mask;
      unsigned int lookup_count, j;

      if (rule->table_type != PANGO_OT_TABLE_GPOS)
	continue;

      mask = rule->property_bit;
      lookup_count = hb_ot_layout_feature_get_lookup_count (info->layout,
							    HB_OT_LAYOUT_TABLE_TYPE_GPOS,
							    rule->feature_index);

      for (j = 0; j < lookup_count; j++)
        {
	  unsigned int lookup_index;

	  lookup_index = hb_ot_layout_feature_get_lookup_index (info->layout,
								HB_OT_LAYOUT_TABLE_TYPE_GPOS,
								rule->feature_index,
								j);
	  hb_ot_layout_position_lookup (info->layout,
					buffer->buffer,
					lookup_index,
					rule->property_bit);
	}

    }

    {
      HB_UInt   i, j;
      HB_Position positions = buffer->buffer->positions;

      /* First handle all left-to-right connections */
      for (j = 0; j < buffer->buffer->in_length; j++)
      {
	if (positions[j].cursive_chain > 0)
	  positions[j].y_pos += positions[j - positions[j].cursive_chain].y_pos;
      }

      /* Then handle all right-to-left connections */
      for (i = buffer->buffer->in_length; i > 0; i--)
      {
	j = i - 1;

	if (positions[j].cursive_chain < 0)
	  positions[j].y_pos += positions[j - positions[j].cursive_chain].y_pos;
      }
    }

   buffer->applied_gpos = TRUE;
}