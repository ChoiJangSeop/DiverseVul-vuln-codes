regset_search_body_position_lead(OnigRegSet* set,
           const UChar* str, const UChar* end,
           const UChar* start, const UChar* range, /* match start range */
           const UChar* orig_range, /* data range */
           OnigOptionType option, MatchArg* msas, int* rmatch_pos)
{
  int r, n, i;
  UChar *s, *prev;
  UChar *low, *high, *low_prev;
  UChar* sch_range;
  regex_t* reg;
  OnigEncoding enc;
  SearchRange* sr;

  n   = set->n;
  enc = set->enc;

  s = (UChar* )start;
  if (s > str)
    prev = onigenc_get_prev_char_head(enc, str, s);
  else
    prev = (UChar* )NULL;

  sr = (SearchRange* )xmalloc(sizeof(*sr) * n);
  CHECK_NULL_RETURN_MEMERR(sr);

  for (i = 0; i < n; i++) {
    reg = set->rs[i].reg;

    sr[i].state = SRS_DEAD;
    if (reg->optimize != OPTIMIZE_NONE) {
      if (reg->dist_max != INFINITE_LEN) {
        sch_range = (UChar* )range + reg->dist_max;
        if (sch_range > end) sch_range = (UChar* )end;

        if (forward_search(reg, str, end, s, sch_range, &low, &high, &low_prev)) {
          sr[i].state = SRS_LOW_HIGH;
          sr[i].low  = low;
          sr[i].high = high;
          sr[i].low_prev = low_prev;
          sr[i].sch_range = sch_range;
        }
      }
      else {
        sch_range = (UChar* )end;
        if (forward_search(reg, str, end, s, sch_range,
                           &low, &high, (UChar** )NULL)) {
          goto total_active;
        }
      }
    }
    else {
    total_active:
      sr[i].state    = SRS_ALL_RANGE;
      sr[i].low      = s;
      sr[i].high     = (UChar* )range;
      sr[i].low_prev = prev;
    }
  }

#define ACTIVATE_ALL_LOW_HIGH_SEARCH_THRESHOLD_LEN   500

  if (set->all_low_high != 0
      && range - start > ACTIVATE_ALL_LOW_HIGH_SEARCH_THRESHOLD_LEN) {
    do {
      int try_count = 0;
      for (i = 0; i < n; i++) {
        if (sr[i].state == SRS_DEAD) continue;

        if (s <  sr[i].low) continue;
        if (s >= sr[i].high) {
          if (forward_search(set->rs[i].reg, str, end, s, sr[i].sch_range,
                             &low, &high, &low_prev) != 0) {
            sr[i].low      = low;
            sr[i].high     = high;
            sr[i].low_prev = low_prev;
            if (s < low) continue;
          }
          else {
            sr[i].state = SRS_DEAD;
            continue;
          }
        }

        reg = set->rs[i].reg;
        REGSET_MATCH_AND_RETURN_CHECK(orig_range);
        try_count++;
      } /* for (i) */

      if (s >= range) break;

      if (try_count == 0) {
        low = (UChar* )range;
        for (i = 0; i < n; i++) {
          if (sr[i].state == SRS_LOW_HIGH && low > sr[i].low) {
            low = sr[i].low;
            low_prev = sr[i].low_prev;
          }
        }
        if (low == range) break;

        s = low;
        prev = low_prev;
      }
      else {
        prev = s;
        s += enclen(enc, s);
      }
    } while (1);
  }
  else {
    int prev_is_newline = 1;
    do {
      for (i = 0; i < n; i++) {
        if (sr[i].state == SRS_DEAD) continue;
        if (sr[i].state == SRS_LOW_HIGH) {
          if (s <  sr[i].low) continue;
          if (s >= sr[i].high) {
            if (forward_search(set->rs[i].reg, str, end, s, sr[i].sch_range,
                               &low, &high, &low_prev) != 0) {
              sr[i].low      = low;
              sr[i].high     = high;
              /* sr[i].low_prev = low_prev; */
              if (s < low) continue;
            }
            else {
              sr[i].state = SRS_DEAD;
              continue;
            }
          }
        }

        reg = set->rs[i].reg;
        if ((reg->anchor & ANCR_ANYCHAR_INF) == 0 || prev_is_newline != 0) {
          REGSET_MATCH_AND_RETURN_CHECK(orig_range);
        }
      }

      if (s >= range) break;

      if (set->anychar_inf != 0)
        prev_is_newline = ONIGENC_IS_MBC_NEWLINE(set->enc, s, end);

      prev = s;
      s += enclen(enc, s);
    } while (1);
  }

  xfree(sr);
  return ONIG_MISMATCH;

 finish:
  xfree(sr);
  return r;

 match:
  xfree(sr);
  *rmatch_pos = (int )(s - str);
  return i;
}