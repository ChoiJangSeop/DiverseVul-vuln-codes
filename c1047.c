addtok (token t)
{
  if (MB_CUR_MAX > 1 && t == MBCSET)
    {
      bool need_or = false;
      struct mb_char_classes *work_mbc = &dfa->mbcsets[dfa->nmbcsets - 1];

      /* Extract wide characters into alternations for better performance.
         This does not require UTF-8.  */
      if (!work_mbc->invert)
        {
          int i;
          for (i = 0; i < work_mbc->nchars; i++)
            {
              addtok_wc (work_mbc->chars[i]);
              if (need_or)
                addtok (OR);
              need_or = true;
            }
          work_mbc->nchars = 0;
        }

      /* UTF-8 allows treating a simple, non-inverted MBCSET like a CSET.  */
      if (work_mbc->invert
          || (!using_utf8() && work_mbc->cset != -1)
          || work_mbc->nchars != 0
          || work_mbc->nch_classes != 0
          || work_mbc->nranges != 0
          || work_mbc->nequivs != 0
          || work_mbc->ncoll_elems != 0)
        {
          addtok_mb (MBCSET, ((dfa->nmbcsets - 1) << 2) + 3);
          if (need_or)
            addtok (OR);
        }
      else
        {
          /* Characters have been handled above, so it is possible
             that the mbcset is empty now.  Do nothing in that case.  */
          if (work_mbc->cset != -1)
            {
              assert (using_utf8 ());
              addtok (CSET + work_mbc->cset);
              if (need_or)
                addtok (OR);
            }
        }
    }
  else
    {
      addtok_mb (t, 3);
    }
}