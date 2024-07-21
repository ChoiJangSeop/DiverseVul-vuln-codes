_hb_ot_layout_init (hb_face_t *face)
{
  hb_ot_layout_t *layout = &face->ot_layout;

  layout->gdef_blob = Sanitizer<GDEF>::sanitize (hb_face_get_table (face, HB_OT_TAG_GDEF));
  layout->gdef = &Sanitizer<GDEF>::lock_instance (layout->gdef_blob);

  layout->gsub_blob = Sanitizer<GSUB>::sanitize (hb_face_get_table (face, HB_OT_TAG_GSUB));
  layout->gsub = &Sanitizer<GSUB>::lock_instance (layout->gsub_blob);

  layout->gpos_blob = Sanitizer<GPOS>::sanitize (hb_face_get_table (face, HB_OT_TAG_GPOS));
  layout->gpos = &Sanitizer<GPOS>::lock_instance (layout->gpos_blob);
}