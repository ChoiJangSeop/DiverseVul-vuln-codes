forward_search_range(regex_t* reg, const UChar* str, const UChar* end, UChar* s,
		     UChar* range, UChar** low, UChar** high, UChar** low_prev)
{
  UChar *p, *pprev = (UChar* )NULL;

#ifdef ONIG_DEBUG_SEARCH
  fprintf(stderr, "forward_search_range: str: %d, end: %d, s: %d, range: %d\n",
	  (int )str, (int )end, (int )s, (int )range);
#endif

  p = s;
  if (reg->dmin > 0) {
    if (ONIGENC_IS_SINGLEBYTE(reg->enc)) {
      p += reg->dmin;
    }
    else {
      UChar *q = p + reg->dmin;
      while (p < q) p += enclen(reg->enc, p);
    }
  }

 retry:
  switch (reg->optimize) {
  case ONIG_OPTIMIZE_EXACT:
    p = slow_search(reg->enc, reg->exact, reg->exact_end, p, end, range);
    break;
  case ONIG_OPTIMIZE_EXACT_IC:
    p = slow_search_ic(reg->enc, reg->case_fold_flag,
                       reg->exact, reg->exact_end, p, end, range);
    break;

  case ONIG_OPTIMIZE_EXACT_BM:
    p = bm_search(reg, reg->exact, reg->exact_end, p, end, range);
    break;

  case ONIG_OPTIMIZE_EXACT_BM_NOT_REV:
    p = bm_search_notrev(reg, reg->exact, reg->exact_end, p, end, range);
    break;

  case ONIG_OPTIMIZE_MAP:
    p = map_search(reg->enc, reg->map, p, range);
    break;
  }

  if (p && p < range) {
    if (p - reg->dmin < s) {
    retry_gate:
      pprev = p;
      p += enclen(reg->enc, p);
      goto retry;
    }

    if (reg->sub_anchor) {
      UChar* prev;

      switch (reg->sub_anchor) {
      case ANCHOR_BEGIN_LINE:
        if (!ON_STR_BEGIN(p)) {
          prev = onigenc_get_prev_char_head(reg->enc,
                                            (pprev ? pprev : str), p);
          if (!ONIGENC_IS_MBC_NEWLINE(reg->enc, prev, end))
            goto retry_gate;
        }
        break;

      case ANCHOR_END_LINE:
        if (ON_STR_END(p)) {
#ifndef USE_NEWLINE_AT_END_OF_STRING_HAS_EMPTY_LINE
          prev = (UChar* )onigenc_get_prev_char_head(reg->enc,
                                                     (pprev ? pprev : str), p);
          if (prev && ONIGENC_IS_MBC_NEWLINE(reg->enc, prev, end))
            goto retry_gate;
#endif
        }
        else if (! ONIGENC_IS_MBC_NEWLINE(reg->enc, p, end)
#ifdef USE_CRNL_AS_LINE_TERMINATOR
                 && ! ONIGENC_IS_MBC_CRNL(reg->enc, p, end)
#endif
                 )
          goto retry_gate;
        break;
      }
    }

    if (reg->dmax == 0) {
      *low = p;
      if (low_prev) {
        if (*low > s)
          *low_prev = onigenc_get_prev_char_head(reg->enc, s, p);
        else
          *low_prev = onigenc_get_prev_char_head(reg->enc,
                                                 (pprev ? pprev : str), p);
      }
    }
    else {
      if (reg->dmax != ONIG_INFINITE_DISTANCE) {
        *low = p - reg->dmax;
        if (*low > s) {
          *low = onigenc_get_right_adjust_char_head_with_prev(reg->enc, s,
                                          *low, (const UChar** )low_prev);
          if (low_prev && IS_NULL(*low_prev))
            *low_prev = onigenc_get_prev_char_head(reg->enc,
                                                   (pprev ? pprev : s), *low);
        }
        else {
          if (low_prev)
            *low_prev = onigenc_get_prev_char_head(reg->enc,
                                                   (pprev ? pprev : str), *low);
        }
      }
    }
    /* no needs to adjust *high, *high is used as range check only */
    *high = p - reg->dmin;

#ifdef ONIG_DEBUG_SEARCH
    fprintf(stderr,
    "forward_search_range success: low: %d, high: %d, dmin: %d, dmax: %d\n",
	    (int )(*low - str), (int )(*high - str), reg->dmin, reg->dmax);
#endif
    return 1; /* success */
  }

  return 0; /* fail */
}