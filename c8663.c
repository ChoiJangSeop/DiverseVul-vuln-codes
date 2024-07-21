add_ctype_to_cc(CClassNode* cc, int ctype, int not, ScanEnv* env)
{
#define ASCII_LIMIT    127

  int c, r;
  int ascii_mode;
  const OnigCodePoint *ranges;
  OnigCodePoint limit;
  OnigCodePoint sb_out;
  OnigEncoding enc = env->enc;

  ascii_mode = IS_ASCII_MODE_CTYPE_OPTION(ctype, env->options);

  r = ONIGENC_GET_CTYPE_CODE_RANGE(enc, ctype, &sb_out, &ranges);
  if (r == 0) {
    if (ascii_mode == 0)
      r = add_ctype_to_cc_by_range(cc, ctype, not, env->enc, sb_out, ranges);
    else
      r = add_ctype_to_cc_by_range_limit(cc, ctype, not, env->enc, sb_out,
                                         ranges, ASCII_LIMIT);
    return r;
  }
  else if (r != ONIG_NO_SUPPORT_CONFIG) {
    return r;
  }

  r = 0;
  limit = ascii_mode ? ASCII_LIMIT : SINGLE_BYTE_SIZE;

  switch (ctype) {
  case ONIGENC_CTYPE_ALPHA:
  case ONIGENC_CTYPE_BLANK:
  case ONIGENC_CTYPE_CNTRL:
  case ONIGENC_CTYPE_DIGIT:
  case ONIGENC_CTYPE_LOWER:
  case ONIGENC_CTYPE_PUNCT:
  case ONIGENC_CTYPE_SPACE:
  case ONIGENC_CTYPE_UPPER:
  case ONIGENC_CTYPE_XDIGIT:
  case ONIGENC_CTYPE_ASCII:
  case ONIGENC_CTYPE_ALNUM:
    if (not != 0) {
      for (c = 0; c < (int )limit; c++) {
        if (! ONIGENC_IS_CODE_CTYPE(enc, (OnigCodePoint )c, ctype))
          BITSET_SET_BIT(cc->bs, c);
      }
      for (c = limit; c < SINGLE_BYTE_SIZE; c++) {
        BITSET_SET_BIT(cc->bs, c);
      }

      ADD_ALL_MULTI_BYTE_RANGE(enc, cc->mbuf);
    }
    else {
      for (c = 0; c < (int )limit; c++) {
        if (ONIGENC_IS_CODE_CTYPE(enc, (OnigCodePoint )c, ctype))
          BITSET_SET_BIT(cc->bs, c);
      }
    }
    break;

  case ONIGENC_CTYPE_GRAPH:
  case ONIGENC_CTYPE_PRINT:
  case ONIGENC_CTYPE_WORD:
    if (not != 0) {
      for (c = 0; c < (int )limit; c++) {
        if (ONIGENC_CODE_TO_MBCLEN(enc, c) > 0 /* check invalid code point */
            && ! ONIGENC_IS_CODE_CTYPE(enc, (OnigCodePoint )c, ctype))
          BITSET_SET_BIT(cc->bs, c);
      }
      for (c = limit; c < SINGLE_BYTE_SIZE; c++) {
        if (ONIGENC_CODE_TO_MBCLEN(enc, c) > 0)
          BITSET_SET_BIT(cc->bs, c);
      }
    }
    else {
      for (c = 0; c < (int )limit; c++) {
        if (ONIGENC_IS_CODE_CTYPE(enc, (OnigCodePoint )c, ctype))
          BITSET_SET_BIT(cc->bs, c);
      }
      if (ascii_mode == 0)
        ADD_ALL_MULTI_BYTE_RANGE(enc, cc->mbuf);
    }
    break;

  default:
    return ONIGERR_PARSER_BUG;
    break;
  }

  return r;
}