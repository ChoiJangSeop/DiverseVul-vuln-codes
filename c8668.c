fetch_name_with_level(OnigCodePoint start_code, UChar** src, UChar* end,
		      UChar** rname_end, ScanEnv* env,
		      int* rback_num, int* rlevel)
{
  int r, sign, is_num, exist_level;
  OnigCodePoint end_code;
  OnigCodePoint c = 0;
  OnigEncoding enc = env->enc;
  UChar *name_end;
  UChar *pnum_head;
  UChar *p = *src;
  PFETCH_READY;

  *rback_num = 0;
  is_num = exist_level = 0;
  sign = 1;
  pnum_head = *src;

  end_code = get_name_end_code_point(start_code);

  name_end = end;
  r = 0;
  if (PEND) {
    return ONIGERR_EMPTY_GROUP_NAME;
  }
  else {
    PFETCH(c);
    if (c == end_code)
      return ONIGERR_EMPTY_GROUP_NAME;

    if (ONIGENC_IS_CODE_DIGIT(enc, c)) {
      is_num = 1;
    }
    else if (c == '-') {
      is_num = 2;
      sign = -1;
      pnum_head = p;
    }
    else if (!ONIGENC_IS_CODE_NAME(enc, c)) {
      r = ONIGERR_INVALID_CHAR_IN_GROUP_NAME;
    }
  }

  while (!PEND) {
    name_end = p;
    PFETCH(c);
    if (c == end_code || c == ')' || c == '+' || c == '-') {
      if (is_num == 2) r = ONIGERR_INVALID_GROUP_NAME;
      break;
    }

    if (is_num != 0) {
      if (ONIGENC_IS_CODE_DIGIT(enc, c)) {
	is_num = 1;
      }
      else {
	r = ONIGERR_INVALID_GROUP_NAME;
	is_num = 0;
      }
    }
    else if (!ONIGENC_IS_CODE_NAME(enc, c)) {
      r = ONIGERR_INVALID_CHAR_IN_GROUP_NAME;
    }
  }

  if (r == 0 && c != end_code) {
    if (c == '+' || c == '-') {
      int level;
      int flag = (c == '-' ? -1 : 1);

      PFETCH(c);
      if (! ONIGENC_IS_CODE_DIGIT(enc, c)) goto err;
      PUNFETCH;
      level = onig_scan_unsigned_number(&p, end, enc);
      if (level < 0) return ONIGERR_TOO_BIG_NUMBER;
      *rlevel = (level * flag);
      exist_level = 1;

      PFETCH(c);
      if (c == end_code)
	goto end;
    }

  err:
    r = ONIGERR_INVALID_GROUP_NAME;
    name_end = end;
  }

 end:
  if (r == 0) {
    if (is_num != 0) {
      *rback_num = onig_scan_unsigned_number(&pnum_head, name_end, enc);
      if (*rback_num < 0) return ONIGERR_TOO_BIG_NUMBER;
      else if (*rback_num == 0) goto err;

      *rback_num *= sign;
    }

    *rname_end = name_end;
    *src = p;
    return (exist_level ? 1 : 0);
  }
  else {
    onig_scan_env_set_error_string(env, r, *src, name_end);
    return r;
  }
}