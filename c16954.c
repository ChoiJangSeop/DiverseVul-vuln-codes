update_line (old, new, current_line, omax, nmax, inv_botlin)
     register char *old, *new;
     int current_line, omax, nmax, inv_botlin;
{
  register char *ofd, *ols, *oe, *nfd, *nls, *ne;
  int temp, lendiff, wsatend, od, nd, twidth, o_cpos;
  int current_invis_chars;
  int col_lendiff, col_temp;
  int bytes_to_insert;
  int mb_cur_max = MB_CUR_MAX;
#if defined (HANDLE_MULTIBYTE)
  mbstate_t ps_new, ps_old;
  int new_offset, old_offset;
#endif

  /* If we're at the right edge of a terminal that supports xn, we're
     ready to wrap around, so do so.  This fixes problems with knowing
     the exact cursor position and cut-and-paste with certain terminal
     emulators.  In this calculation, TEMP is the physical screen
     position of the cursor. */
  if (mb_cur_max > 1 && rl_byte_oriented == 0)
    temp = _rl_last_c_pos;
  else
    temp = _rl_last_c_pos - WRAP_OFFSET (_rl_last_v_pos, visible_wrap_offset);
  if (temp == _rl_screenwidth && _rl_term_autowrap && !_rl_horizontal_scroll_mode
	&& _rl_last_v_pos == current_line - 1)
    {
#if defined (HANDLE_MULTIBYTE)
      if (mb_cur_max > 1 && rl_byte_oriented == 0)
	{
	  wchar_t wc;
	  mbstate_t ps;
	  int tempwidth, bytes;
	  size_t ret;

	  /* This fixes only double-column characters, but if the wrapped
	     character consumes more than three columns, spaces will be
	     inserted in the string buffer. */
	  if (current_line < line_state_visible->wbsize && line_state_visible->wrapped_line[current_line] > 0)
	    _rl_clear_to_eol (line_state_visible->wrapped_line[current_line]);

	  memset (&ps, 0, sizeof (mbstate_t));
	  ret = mbrtowc (&wc, new, mb_cur_max, &ps);
	  if (MB_INVALIDCH (ret))
	    {
	      tempwidth = 1;
	      ret = 1;
	    }
	  else if (MB_NULLWCH (ret))
	    tempwidth = 0;
	  else
	    tempwidth = WCWIDTH (wc);

	  if (tempwidth > 0)
	    {
	      int count, i;
	      bytes = ret;
	      for (count = 0; count < bytes; count++)
		putc (new[count], rl_outstream);
	      _rl_last_c_pos = tempwidth;
	      _rl_last_v_pos++;
	      memset (&ps, 0, sizeof (mbstate_t));
	      ret = mbrtowc (&wc, old, mb_cur_max, &ps);
	      if (ret != 0 && bytes != 0)
		{
		  if (MB_INVALIDCH (ret))
		    ret = 1;
		  memmove (old+bytes, old+ret, strlen (old+ret));
		  memcpy (old, new, bytes);
		  /* Fix up indices if we copy data from one line to another */
		  omax += bytes - ret;
		  for (i = current_line+1; i <= inv_botlin+1; i++)
		    vis_lbreaks[i] += bytes - ret;
		}
	    }
	  else
	    {
	      putc (' ', rl_outstream);
	      _rl_last_c_pos = 1;
	      _rl_last_v_pos++;
	      if (old[0] && new[0])
		old[0] = new[0];
	    }
	}
      else
#endif
	{
	  if (new[0])
	    putc (new[0], rl_outstream);
	  else
	    putc (' ', rl_outstream);
	  _rl_last_c_pos = 1;
	  _rl_last_v_pos++;
	  if (old[0] && new[0])
	    old[0] = new[0];
	}
    }

      
  /* Find first difference. */
#if defined (HANDLE_MULTIBYTE)
  if (mb_cur_max > 1 && rl_byte_oriented == 0)
    {
      /* See if the old line is a subset of the new line, so that the
	 only change is adding characters. */
      temp = (omax < nmax) ? omax : nmax;
      if (memcmp (old, new, temp) == 0)		/* adding at the end */
	{
	  new_offset = old_offset = temp;
	  ofd = old + temp;
	  nfd = new + temp;
	}
      else
	{      
	  memset (&ps_new, 0, sizeof(mbstate_t));
	  memset (&ps_old, 0, sizeof(mbstate_t));

	  if (omax == nmax && STREQN (new, old, omax))
	    {
	      old_offset = omax;
	      new_offset = nmax;
	      ofd = old + omax;
	      nfd = new + nmax;
	    }
	  else
	    {
	      new_offset = old_offset = 0;
	      for (ofd = old, nfd = new;
		    (ofd - old < omax) && *ofd &&
		    _rl_compare_chars(old, old_offset, &ps_old, new, new_offset, &ps_new); )
		{
		  old_offset = _rl_find_next_mbchar (old, old_offset, 1, MB_FIND_ANY);
		  new_offset = _rl_find_next_mbchar (new, new_offset, 1, MB_FIND_ANY);

		  ofd = old + old_offset;
		  nfd = new + new_offset;
		}
	    }
	}
    }
  else
#endif
  for (ofd = old, nfd = new;
       (ofd - old < omax) && *ofd && (*ofd == *nfd);
       ofd++, nfd++)
    ;

  /* Move to the end of the screen line.  ND and OD are used to keep track
     of the distance between ne and new and oe and old, respectively, to
     move a subtraction out of each loop. */
  for (od = ofd - old, oe = ofd; od < omax && *oe; oe++, od++);
  for (nd = nfd - new, ne = nfd; nd < nmax && *ne; ne++, nd++);

  /* If no difference, continue to next line. */
  if (ofd == oe && nfd == ne)
    return;

#if defined (HANDLE_MULTIBYTE)
  if (mb_cur_max > 1 && rl_byte_oriented == 0 && _rl_utf8locale)
    {
      wchar_t wc;
      mbstate_t ps = { 0 };
      int t;

      /* If the first character in the difference is a zero-width character,
	 assume it's a combining character and back one up so the two base
	 characters no longer compare equivalently. */
      t = mbrtowc (&wc, ofd, mb_cur_max, &ps);
      if (t > 0 && UNICODE_COMBINING_CHAR (wc) && WCWIDTH (wc) == 0)
	{
	  old_offset = _rl_find_prev_mbchar (old, ofd - old, MB_FIND_ANY);
	  new_offset = _rl_find_prev_mbchar (new, nfd - new, MB_FIND_ANY);
	  ofd = old + old_offset;	/* equal by definition */
	  nfd = new + new_offset;
	}
    }
#endif

  wsatend = 1;			/* flag for trailing whitespace */

#if defined (HANDLE_MULTIBYTE)
  if (mb_cur_max > 1 && rl_byte_oriented == 0)
    {
      ols = old + _rl_find_prev_mbchar (old, oe - old, MB_FIND_ANY);
      nls = new + _rl_find_prev_mbchar (new, ne - new, MB_FIND_ANY);

      while ((ols > ofd) && (nls > nfd))
	{
	  memset (&ps_old, 0, sizeof (mbstate_t));
	  memset (&ps_new, 0, sizeof (mbstate_t));

#if 0
	  /* On advice from jir@yamato.ibm.com */
	  _rl_adjust_point (old, ols - old, &ps_old);
	  _rl_adjust_point (new, nls - new, &ps_new);
#endif

	  if (_rl_compare_chars (old, ols - old, &ps_old, new, nls - new, &ps_new) == 0)
	    break;

	  if (*ols == ' ')
	    wsatend = 0;

	  ols = old + _rl_find_prev_mbchar (old, ols - old, MB_FIND_ANY);
	  nls = new + _rl_find_prev_mbchar (new, nls - new, MB_FIND_ANY);
	}
    }
  else
    {
#endif /* HANDLE_MULTIBYTE */
  ols = oe - 1;			/* find last same */
  nls = ne - 1;
  while ((ols > ofd) && (nls > nfd) && (*ols == *nls))
    {
      if (*ols != ' ')
	wsatend = 0;
      ols--;
      nls--;
    }
#if defined (HANDLE_MULTIBYTE)
    }
#endif

  if (wsatend)
    {
      ols = oe;
      nls = ne;
    }
#if defined (HANDLE_MULTIBYTE)
  /* This may not work for stateful encoding, but who cares?  To handle
     stateful encoding properly, we have to scan each string from the
     beginning and compare. */
  else if (_rl_compare_chars (ols, 0, NULL, nls, 0, NULL) == 0)
#else
  else if (*ols != *nls)
#endif
    {
      if (*ols)			/* don't step past the NUL */
	{
	  if (mb_cur_max > 1 && rl_byte_oriented == 0)
	    ols = old + _rl_find_next_mbchar (old, ols - old, 1, MB_FIND_ANY);
	  else
	    ols++;
	}
      if (*nls)
	{
	  if (mb_cur_max > 1 && rl_byte_oriented == 0)
	    nls = new + _rl_find_next_mbchar (new, nls - new, 1, MB_FIND_ANY);
	  else
	    nls++;
	}
    }

  /* count of invisible characters in the current invisible line. */
  current_invis_chars = W_OFFSET (current_line, wrap_offset);
  if (_rl_last_v_pos != current_line)
    {
      _rl_move_vert (current_line);
      /* We have moved up to a new screen line.  This line may or may not have
         invisible characters on it, but we do our best to recalculate
         visible_wrap_offset based on what we know. */
      if (current_line == 0)
	visible_wrap_offset = prompt_invis_chars_first_line;	/* XXX */
      if ((mb_cur_max == 1 || rl_byte_oriented) && current_line == 0 && visible_wrap_offset)
	_rl_last_c_pos += visible_wrap_offset;
    }

  /* If this is the first line and there are invisible characters in the
     prompt string, and the prompt string has not changed, and the current
     cursor position is before the last invisible character in the prompt,
     and the index of the character to move to is past the end of the prompt
     string, then redraw the entire prompt string.  We can only do this
     reliably if the terminal supports a `cr' capability.

     This can also happen if the prompt string has changed, and the first
     difference in the line is in the middle of the prompt string, after a
     sequence of invisible characters (worst case) and before the end of
     the prompt.  In this case, we have to redraw the entire prompt string
     so that the entire sequence of invisible characters is drawn.  We need
     to handle the worst case, when the difference is after (or in the middle
     of) a sequence of invisible characters that changes the text color and
     before the sequence that restores the text color to normal.  Then we have
     to make sure that the lines still differ -- if they don't, we can
     return immediately.

     This is not an efficiency hack -- there is a problem with redrawing
     portions of the prompt string if they contain terminal escape
     sequences (like drawing the `unbold' sequence without a corresponding
     `bold') that manifests itself on certain terminals. */

  lendiff = local_prompt_len;
  if (lendiff > nmax)
    lendiff = nmax;
  od = ofd - old;	/* index of first difference in visible line */
  nd = nfd - new;
  if (current_line == 0 && !_rl_horizontal_scroll_mode &&
      _rl_term_cr && lendiff > prompt_visible_length && _rl_last_c_pos > 0 &&
      (((od > 0 || nd > 0) && (od < PROMPT_ENDING_INDEX || nd < PROMPT_ENDING_INDEX)) ||
		((od >= lendiff) && _rl_last_c_pos < PROMPT_ENDING_INDEX)))
    {
#if defined (__MSDOS__)
      putc ('\r', rl_outstream);
#else
      tputs (_rl_term_cr, 1, _rl_output_character_function);
#endif
      if (modmark)
	_rl_output_some_chars ("*", 1);
      _rl_output_some_chars (local_prompt, lendiff);
      if (mb_cur_max > 1 && rl_byte_oriented == 0)
	{
	  /* We take wrap_offset into account here so we can pass correct
	     information to _rl_move_cursor_relative. */
	  _rl_last_c_pos = _rl_col_width (local_prompt, 0, lendiff, 1) - wrap_offset + modmark;
	  cpos_adjusted = 1;
	}
      else
	_rl_last_c_pos = lendiff + modmark;

      /* Now if we have printed the prompt string because the first difference
	 was within the prompt, see if we need to recompute where the lines
	 differ.  Check whether where we are now is past the last place where
	 the old and new lines are the same and short-circuit now if we are. */
      if ((od < PROMPT_ENDING_INDEX || nd < PROMPT_ENDING_INDEX) &&
          omax == nmax &&
	  lendiff > (ols-old) && lendiff > (nls-new))
	return;

      /* XXX - we need to fix up our calculations if we are now past the
	 old ofd/nfd and the prompt length (or line length) has changed.
	 We punt on the problem and do a dumb update.  We'd like to be able
	 to just output the prompt from the beginning of the line up to the
	 first difference, but you don't know the number of invisible
	 characters in that case.
	 This needs a lot of work to be efficient. */
      if ((od < PROMPT_ENDING_INDEX || nd < PROMPT_ENDING_INDEX))
	{
	  nfd = new + lendiff;	/* number of characters we output above */
	  nd = lendiff;

	  /* Do a dumb update and return */
	  temp = ne - nfd;
	  if (temp > 0)
	    {
	      _rl_output_some_chars (nfd, temp);
	      if (mb_cur_max > 1 && rl_byte_oriented == 0)
		_rl_last_c_pos += _rl_col_width (new, nd, ne - new, 1);
	      else
		_rl_last_c_pos += temp;
	    }
	  if (nmax < omax)
	    goto clear_rest_of_line;	/* XXX */
	  else
	    return;
	}
    }

  o_cpos = _rl_last_c_pos;

  /* When this function returns, _rl_last_c_pos is correct, and an absolute
     cursor position in multibyte mode, but a buffer index when not in a
     multibyte locale. */
  _rl_move_cursor_relative (od, old);

#if defined (HANDLE_MULTIBYTE)
  /* We need to indicate that the cursor position is correct in the presence of
     invisible characters in the prompt string.  Let's see if setting this when
     we make sure we're at the end of the drawn prompt string works. */
  if (current_line == 0 && mb_cur_max > 1 && rl_byte_oriented == 0 &&
      (_rl_last_c_pos > 0 || o_cpos > 0) &&
      _rl_last_c_pos == prompt_physical_chars)
    cpos_adjusted = 1;
#endif

  /* if (len (new) > len (old))
     lendiff == difference in buffer (bytes)
     col_lendiff == difference on screen (columns)
     When not using multibyte characters, these are equal */
  lendiff = (nls - nfd) - (ols - ofd);
  if (mb_cur_max > 1 && rl_byte_oriented == 0)
    col_lendiff = _rl_col_width (new, nfd - new, nls - new, 1) - _rl_col_width (old, ofd - old, ols - old, 1);
  else
    col_lendiff = lendiff;

  /* If we are changing the number of invisible characters in a line, and
     the spot of first difference is before the end of the invisible chars,
     lendiff needs to be adjusted. */
  if (current_line == 0 && /* !_rl_horizontal_scroll_mode && */
      current_invis_chars != visible_wrap_offset)
    {
      if (mb_cur_max > 1 && rl_byte_oriented == 0)
	{
	  lendiff += visible_wrap_offset - current_invis_chars;
	  col_lendiff += visible_wrap_offset - current_invis_chars;
	}
      else
	{
	  lendiff += visible_wrap_offset - current_invis_chars;
	  col_lendiff = lendiff;
	}
    }

  /* We use temp as a count of the number of bytes from the first difference
     to the end of the new line.  col_temp is the corresponding number of
     screen columns.  A `dumb' update moves to the spot of first difference
     and writes TEMP bytes. */
  /* Insert (diff (len (old), len (new)) ch. */
  temp = ne - nfd;
  if (mb_cur_max > 1 && rl_byte_oriented == 0)
    col_temp = _rl_col_width (new, nfd - new, ne - new, 1);
  else
    col_temp = temp;

  /* how many bytes from the new line buffer to write to the display */
  bytes_to_insert = nls - nfd;

  /* col_lendiff > 0 if we are adding characters to the line */
  if (col_lendiff > 0)	/* XXX - was lendiff */
    {
      /* Non-zero if we're increasing the number of lines. */
      int gl = current_line >= _rl_vis_botlin && inv_botlin > _rl_vis_botlin;
      /* If col_lendiff is > 0, implying that the new string takes up more
	 screen real estate than the old, but lendiff is < 0, meaning that it
	 takes fewer bytes, we need to just output the characters starting
	 from the first difference.  These will overwrite what is on the
	 display, so there's no reason to do a smart update.  This can really
	 only happen in a multibyte environment. */
      if (lendiff < 0)
	{
	  _rl_output_some_chars (nfd, temp);
	  _rl_last_c_pos += col_temp;	/* XXX - was _rl_col_width (nfd, 0, temp, 1); */
	  /* If nfd begins before any invisible characters in the prompt,
	     adjust _rl_last_c_pos to account for wrap_offset and set
	     cpos_adjusted to let the caller know. */
	  if (current_line == 0 && displaying_prompt_first_line && wrap_offset && ((nfd - new) <= prompt_last_invisible))
	    {
	      _rl_last_c_pos -= wrap_offset;
	      cpos_adjusted = 1;
	    }
	  return;
	}
      /* Sometimes it is cheaper to print the characters rather than
	 use the terminal's capabilities.  If we're growing the number
	 of lines, make sure we actually cause the new line to wrap
	 around on auto-wrapping terminals. */
      else if (_rl_terminal_can_insert && ((2 * col_temp) >= col_lendiff || _rl_term_IC) && (!_rl_term_autowrap || !gl))
	{
	  /* If lendiff > prompt_visible_length and _rl_last_c_pos == 0 and
	     _rl_horizontal_scroll_mode == 1, inserting the characters with
	     _rl_term_IC or _rl_term_ic will screw up the screen because of the
	     invisible characters.  We need to just draw them. */
	  /* The same thing happens if we're trying to draw before the last
	     invisible character in the prompt string or we're increasing the
	     number of invisible characters in the line and we're not drawing
	     the entire prompt string. */
	  if (*ols && ((_rl_horizontal_scroll_mode &&
			_rl_last_c_pos == 0 &&
			lendiff > prompt_visible_length &&
			current_invis_chars > 0) == 0) &&
		      (((mb_cur_max > 1 && rl_byte_oriented == 0) &&
		        current_line == 0 && wrap_offset &&
		        ((nfd - new) <= prompt_last_invisible) &&
		        (col_lendiff < prompt_visible_length)) == 0) &&
		      (visible_wrap_offset >= current_invis_chars))
	    {
	      open_some_spaces (col_lendiff);
	      _rl_output_some_chars (nfd, bytes_to_insert);
	      if (mb_cur_max > 1 && rl_byte_oriented == 0)
		_rl_last_c_pos += _rl_col_width (nfd, 0, bytes_to_insert, 1);
	      else
		_rl_last_c_pos += bytes_to_insert;
	    }
	  else if ((mb_cur_max == 1 || rl_byte_oriented != 0) && *ols == 0 && lendiff > 0)
	    {
	      /* At the end of a line the characters do not have to
		 be "inserted".  They can just be placed on the screen. */
	      _rl_output_some_chars (nfd, temp);
	      _rl_last_c_pos += col_temp;
	      return;
	    }
	  else	/* just write from first difference to end of new line */
	    {
	      _rl_output_some_chars (nfd, temp);
	      _rl_last_c_pos += col_temp;
	      /* If nfd begins before the last invisible character in the
		 prompt, adjust _rl_last_c_pos to account for wrap_offset
		 and set cpos_adjusted to let the caller know. */
	      if ((mb_cur_max > 1 && rl_byte_oriented == 0) && current_line == 0 && displaying_prompt_first_line && wrap_offset && ((nfd - new) <= prompt_last_invisible))
		{
		  _rl_last_c_pos -= wrap_offset;
		  cpos_adjusted = 1;
		}
	      return;
	    }

	  if (bytes_to_insert > lendiff)
	    {
	      /* If nfd begins before the last invisible character in the
		 prompt, adjust _rl_last_c_pos to account for wrap_offset
		 and set cpos_adjusted to let the caller know. */
	      if ((mb_cur_max > 1 && rl_byte_oriented == 0) && current_line == 0 && displaying_prompt_first_line && wrap_offset && ((nfd - new) <= prompt_last_invisible))
		{
		  _rl_last_c_pos -= wrap_offset;
		  cpos_adjusted = 1;
		}
	    }
	}
      else
	{
	  /* cannot insert chars, write to EOL */
	  _rl_output_some_chars (nfd, temp);
	  _rl_last_c_pos += col_temp;
	  /* If we're in a multibyte locale and were before the last invisible
	     char in the current line (which implies we just output some invisible
	     characters) we need to adjust _rl_last_c_pos, since it represents
	     a physical character position. */
	  /* The current_line*rl_screenwidth+prompt_invis_chars_first_line is a
	     crude attempt to compute how far into the new line buffer we are.
	     It doesn't work well in the face of multibyte characters and needs
	     to be rethought. XXX */
	  if ((mb_cur_max > 1 && rl_byte_oriented == 0) &&
		current_line == prompt_last_screen_line && wrap_offset &&
		displaying_prompt_first_line &&
		wrap_offset != prompt_invis_chars_first_line &&
		((nfd-new) < (prompt_last_invisible-(current_line*_rl_screenwidth+prompt_invis_chars_first_line))))
	    {
	      _rl_last_c_pos -= wrap_offset - prompt_invis_chars_first_line;
	      cpos_adjusted = 1;
	    }
	}
    }
  else				/* Delete characters from line. */
    {
      /* If possible and inexpensive to use terminal deletion, then do so. */
      if (_rl_term_dc && (2 * col_temp) >= -col_lendiff)
	{
	  /* If all we're doing is erasing the invisible characters in the
	     prompt string, don't bother.  It screws up the assumptions
	     about what's on the screen. */
	  if (_rl_horizontal_scroll_mode && _rl_last_c_pos == 0 &&
	      displaying_prompt_first_line &&
	      -lendiff == visible_wrap_offset)
	    col_lendiff = 0;

	  /* If we have moved lmargin and we're shrinking the line, we've
	     already moved the cursor to the first character of the new line,
	     so deleting -col_lendiff characters will mess up the cursor
	     position calculation */
	  if (_rl_horizontal_scroll_mode && displaying_prompt_first_line == 0 &&
		col_lendiff && _rl_last_c_pos < -col_lendiff)
	    col_lendiff = 0;

	  if (col_lendiff)
	    delete_chars (-col_lendiff); /* delete (diff) characters */

	  /* Copy (new) chars to screen from first diff to last match,
	     overwriting what is there. */
	  if (bytes_to_insert > 0)
	    {
	      /* If nfd begins at the prompt, or before the invisible
		 characters in the prompt, we need to adjust _rl_last_c_pos
		 in a multibyte locale to account for the wrap offset and
		 set cpos_adjusted accordingly. */
	      _rl_output_some_chars (nfd, bytes_to_insert);
	      if (mb_cur_max > 1 && rl_byte_oriented == 0)
		{
		  _rl_last_c_pos += _rl_col_width (nfd, 0, bytes_to_insert, 1);
		  if (current_line == 0 && wrap_offset &&
			displaying_prompt_first_line &&
			_rl_last_c_pos > wrap_offset &&
			((nfd - new) <= prompt_last_invisible))
		    {
		      _rl_last_c_pos -= wrap_offset;
		      cpos_adjusted = 1;
		    }
		}
	      else
		_rl_last_c_pos += bytes_to_insert;

	      /* XXX - we only want to do this if we are at the end of the line
		 so we move there with _rl_move_cursor_relative */
	      if (_rl_horizontal_scroll_mode && ((oe-old) > (ne-new)))
		{
		  _rl_move_cursor_relative (ne-new, new);
		  goto clear_rest_of_line;
		}
	    }
	}
      /* Otherwise, print over the existing material. */
      else
	{
	  if (temp > 0)
	    {
	      /* If nfd begins at the prompt, or before the invisible
		 characters in the prompt, we need to adjust _rl_last_c_pos
		 in a multibyte locale to account for the wrap offset and
		 set cpos_adjusted accordingly. */
	      _rl_output_some_chars (nfd, temp);
	      _rl_last_c_pos += col_temp;		/* XXX */
	      if (mb_cur_max > 1 && rl_byte_oriented == 0)
		{
		  if (current_line == 0 && wrap_offset &&
			displaying_prompt_first_line &&
			_rl_last_c_pos > wrap_offset &&
			((nfd - new) <= prompt_last_invisible))
		    {
		      _rl_last_c_pos -= wrap_offset;
		      cpos_adjusted = 1;
		    }
		}
	    }
clear_rest_of_line:
	  lendiff = (oe - old) - (ne - new);
	  if (mb_cur_max > 1 && rl_byte_oriented == 0)
	    col_lendiff = _rl_col_width (old, 0, oe - old, 1) - _rl_col_width (new, 0, ne - new, 1);
	  else
	    col_lendiff = lendiff;

	  /* If we've already printed over the entire width of the screen,
	     including the old material, then col_lendiff doesn't matter and
	     space_to_eol will insert too many spaces.  XXX - maybe we should
	     adjust col_lendiff based on the difference between _rl_last_c_pos
	     and _rl_screenwidth */
	  if (col_lendiff && ((mb_cur_max == 1 || rl_byte_oriented) || (_rl_last_c_pos < _rl_screenwidth)))
	    {	  
	      if (_rl_term_autowrap && current_line < inv_botlin)
		space_to_eol (col_lendiff);
	      else
		_rl_clear_to_eol (col_lendiff);
	    }
	}
    }
}