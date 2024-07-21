forward_search(regex_t* reg, const UChar* str, const UChar* end, UChar* start,
               UChar* range, UChar** low, UChar** high, UChar** low_prev)
{
  UChar *p, *pprev = (UChar* )NULL;

#ifdef ONIG_DEBUG_SEARCH
  fprintf(stderr, "forward_search: str: %p, end: %p, start: %p, range: %p\n",
          str, end, start, range);
#endif

  p = start;
  if (reg->dist_min > 0) {
    if (ONIGENC_IS_SINGLEBYTE(reg->enc)) {
      p += reg->dist_min;
    }
    else {
      UChar *q = p + reg->dist_min;

      if (q >= end) return 0; /* fail */
      while (p < q) p += enclen(reg->enc, p);
    }
  }

 retry:
  switch (reg->optimize) {
  case OPTIMIZE_STR:
    p = slow_search(reg->enc, reg->exact, reg->exact_end, p, end, range);
    break;
  case OPTIMIZE_STR_CASE_FOLD:
    p = slow_search_ic(reg->enc, reg->case_fold_flag,
                       reg->exact, reg->exact_end, p, end, range);
    break;

  case OPTIMIZE_STR_FAST:
    p = sunday_quick_search(reg, reg->exact, reg->exact_end, p, end, range);
    break;

  case OPTIMIZE_STR_FAST_STEP_FORWARD:
    p = sunday_quick_search_step_forward(reg, reg->exact, reg->exact_end,
                                         p, end, range);
    break;

  case OPTIMIZE_MAP:
    p = map_search(reg->enc, reg->map, p, range);
    break;
  }

  if (p && p < range) {
    if (p - reg->dist_min < start) {
    retry_gate:
      pprev = p;
      p += enclen(reg->enc, p);
      goto retry;
    }

    if (reg->sub_anchor) {
      UChar* prev;

      switch (reg->sub_anchor) {
      case ANCR_BEGIN_LINE:
        if (!ON_STR_BEGIN(p)) {
          prev = onigenc_get_prev_char_head(reg->enc, (pprev ? pprev : str), p);
          if (!ONIGENC_IS_MBC_NEWLINE(reg->enc, prev, end))
            goto retry_gate;
        }
        break;

      case ANCR_END_LINE:
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

    if (reg->dist_max == 0) {
      *low = p;
      if (low_prev) {
        if (*low > start)
          *low_prev = onigenc_get_prev_char_head(reg->enc, start, p);
        else
          *low_prev = onigenc_get_prev_char_head(reg->enc,
                                                 (pprev ? pprev : str), p);
      }
    }
    else {
      if (reg->dist_max != INFINITE_LEN) {
        if (p - str < reg->dist_max) {
          *low = (UChar* )str;
          if (low_prev)
            *low_prev = onigenc_get_prev_char_head(reg->enc, str, *low);
        }
        else {
          *low = p - reg->dist_max;
          if (*low > start) {
            *low = onigenc_get_right_adjust_char_head_with_prev(reg->enc, start,
                                                 *low, (const UChar** )low_prev);
          }
          else {
            if (low_prev)
              *low_prev = onigenc_get_prev_char_head(reg->enc,
                                                     (pprev ? pprev : str), *low);
          }
        }
      }
    }
    /* no needs to adjust *high, *high is used as range check only */
    *high = p - reg->dist_min;

#ifdef ONIG_DEBUG_SEARCH
    fprintf(stderr,
            "forward_search success: low: %d, high: %d, dmin: %d, dmax: %d\n",
            (int )(*low - str), (int )(*high - str),
            reg->dist_min, reg->dist_max);
#endif
    return 1; /* success */
  }

  return 0; /* fail */
}