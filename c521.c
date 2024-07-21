set_unicode_charmap (FT_Face face)
{
  int charmap;

  for (charmap = 0; charmap < face->num_charmaps; charmap++)
    if (face->charmaps[charmap]->encoding == ft_encoding_unicode)
      {
	HB_Error error = FT_Set_Charmap(face, face->charmaps[charmap]);
	return error == HB_Err_Ok;
      }

  return FALSE;
}