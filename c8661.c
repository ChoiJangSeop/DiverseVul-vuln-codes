backward_search(regex_t* reg, const UChar* str, const UChar* end, UChar* s,
                const UChar* range, UChar* adjrange, UChar** low, UChar** high)
{
  UChar *p;

  p = s;

 retry:
  switch (reg->optimize) {
  case OPTIMIZE_STR:
  exact_method:
    p = slow_search_backward(reg->enc, reg->exact, reg->exact_end,
                             range, adjrange, end, p);
    break;

  case OPTIMIZE_STR_CASE_FOLD:
    p = slow_search_backward_ic(reg->enc, reg->case_fold_flag,
                                reg->exact, reg->exact_end,
                                range, adjrange, end, p);
    break;

  case OPTIMIZE_STR_FAST:
  case OPTIMIZE_STR_FAST_STEP_FORWARD:
    goto exact_method;
    break;

  case OPTIMIZE_MAP:
    p = map_search_backward(reg->enc, reg->map, range, adjrange, p);
    break;
  }

  if (p) {
    if (reg->sub_anchor) {
      UChar* prev;

      switch (reg->sub_anchor) {
      case ANCR_BEGIN_LINE:
        if (!ON_STR_BEGIN(p)) {
          prev = onigenc_get_prev_char_head(reg->enc, str, p);
          if (IS_NOT_NULL(prev) && !ONIGENC_IS_MBC_NEWLINE(reg->enc, prev, end)) {
            p = prev;
            goto retry;
          }
        }
        break;

      case ANCR_END_LINE:
        if (ON_STR_END(p)) {
#ifndef USE_NEWLINE_AT_END_OF_STRING_HAS_EMPTY_LINE
          prev = onigenc_get_prev_char_head(reg->enc, adjrange, p);
          if (IS_NULL(prev)) goto fail;
          if (ONIGENC_IS_MBC_NEWLINE(reg->enc, prev, end)) {
            p = prev;
            goto retry;
          }
#endif
        }
        else if (! ONIGENC_IS_MBC_NEWLINE(reg->enc, p, end)
#ifdef USE_CRNL_AS_LINE_TERMINATOR
                 && ! ONIGENC_IS_MBC_CRNL(reg->enc, p, end)
#endif
                 ) {
          p = onigenc_get_prev_char_head(reg->enc, adjrange, p);
          if (IS_NULL(p)) goto fail;
          goto retry;
        }
        break;
      }
    }

    if (reg->dist_max != INFINITE_LEN) {
      if ((ptrdiff_t )(p - str) < (ptrdiff_t )reg->dist_max)
        *low = (UChar* )str;
      else
        *low = p - reg->dist_max;

      if (reg->dist_min != 0) {
        if ((ptrdiff_t )(p - str) < (ptrdiff_t )reg->dist_min)
          *high = (UChar* )str;
        else
          *high = p - reg->dist_min;
      }
      else {
        *high = p;
      }

      *high = onigenc_get_right_adjust_char_head(reg->enc, adjrange, *high);
    }

#ifdef ONIG_DEBUG_SEARCH
    fprintf(stderr, "backward_search: low: %d, high: %d\n",
            (int )(*low - str), (int )(*high - str));
#endif
    return 1; /* success */
  }

 fail:
#ifdef ONIG_DEBUG_SEARCH
  fprintf(stderr, "backward_search: fail.\n");
#endif
  return 0; /* fail */
}