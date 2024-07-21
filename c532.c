get_glyph_class (gunichar   charcode,
		 HB_UShort *class)
{
  /* For characters mapped into the Arabic Presentation forms, using properties
   * derived as we apply GSUB substitutions will be more reliable
   */
  if ((charcode >= 0xFB50 && charcode <= 0xFDFF) || /* Arabic Presentation Forms-A */
      (charcode >= 0xFE70 && charcode <= 0XFEFF))   /* Arabic Presentation Forms-B */
    return FALSE;

  switch ((int) g_unichar_type (charcode))
    {
    case G_UNICODE_COMBINING_MARK:
    case G_UNICODE_ENCLOSING_MARK:
    case G_UNICODE_NON_SPACING_MARK:
      *class = 3;		/* Mark glyph (non-spacing combining glyph) */
      return TRUE;
    case G_UNICODE_UNASSIGNED:
    case G_UNICODE_PRIVATE_USE:
      return FALSE;		/* Unknown, don't assign a class; classes get
				 * propagated during GSUB application */
    default:
      *class = 1;               /* Base glyph (single character, spacing glyph) */
      return TRUE;
    }
}