keystr_from_desc(KEYDB_SEARCH_DESC *desc)
{
  switch(desc->mode)
    {
    case KEYDB_SEARCH_MODE_LONG_KID:
    case KEYDB_SEARCH_MODE_SHORT_KID:
      return keystr(desc->u.kid);

    case KEYDB_SEARCH_MODE_FPR20:
      {
	u32 keyid[2];

	keyid[0] = ((unsigned char)desc->u.fpr[12] << 24
                    | (unsigned char)desc->u.fpr[13] << 16
                    | (unsigned char)desc->u.fpr[14] << 8
                    | (unsigned char)desc->u.fpr[15]);
	keyid[1] = ((unsigned char)desc->u.fpr[16] << 24
                    | (unsigned char)desc->u.fpr[17] << 16
                    | (unsigned char)desc->u.fpr[18] << 8
                    | (unsigned char)desc->u.fpr[19]);

	return keystr(keyid);
      }

    case KEYDB_SEARCH_MODE_FPR16:
      return "?v3 fpr?";

    default:
      BUG();
    }
}