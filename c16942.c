rl_callback_read_char ()
{
  char *line;
  int eof, jcode;
  static procenv_t olevel;

  if (rl_linefunc == NULL)
    {
      _rl_errmsg ("readline_callback_read_char() called with no handler!");
      abort ();
    }

  memcpy ((void *)olevel, (void *)_rl_top_level, sizeof (procenv_t));
#if defined (HAVE_POSIX_SIGSETJMP)
  jcode = sigsetjmp (_rl_top_level, 0);
#else
  jcode = setjmp (_rl_top_level);
#endif
  if (jcode)
    {
      (*rl_redisplay_function) ();
      _rl_want_redisplay = 0;
      memcpy ((void *)_rl_top_level, (void *)olevel, sizeof (procenv_t));
      CALLBACK_READ_RETURN ();
    }

#if defined (HANDLE_SIGNALS)
  /* Install signal handlers only when readline has control. */
  rl_set_signals ();
#endif

  do
    {
      RL_CHECK_SIGNALS ();
      if  (RL_ISSTATE (RL_STATE_ISEARCH))
	{
	  eof = _rl_isearch_callback (_rl_iscxt);
	  if (eof == 0 && (RL_ISSTATE (RL_STATE_ISEARCH) == 0) && RL_ISSTATE (RL_STATE_INPUTPENDING))
	    rl_callback_read_char ();

	  CALLBACK_READ_RETURN ();
	}
      else if  (RL_ISSTATE (RL_STATE_NSEARCH))
	{
	  eof = _rl_nsearch_callback (_rl_nscxt);

	  CALLBACK_READ_RETURN ();
	}
#if defined (VI_MODE)
      /* States that can occur while in state VIMOTION have to be checked
	 before RL_STATE_VIMOTION */
      else if (RL_ISSTATE (RL_STATE_CHARSEARCH))
	{
	  int k;

	  k = _rl_callback_data->i2;

	  eof = (*_rl_callback_func) (_rl_callback_data);
	  /* If the function `deregisters' itself, make sure the data is
	     cleaned up. */
	  if (_rl_callback_func == 0)	/* XXX - just sanity check */
	    {
	      if (_rl_callback_data)
		{
		  _rl_callback_data_dispose (_rl_callback_data);
		  _rl_callback_data = 0;
		}
	    }

	  /* Messy case where vi motion command can be char search */
	  if (RL_ISSTATE (RL_STATE_VIMOTION))
	    {
	      _rl_vi_domove_motion_cleanup (k, _rl_vimvcxt);
	      _rl_internal_char_cleanup ();
	      CALLBACK_READ_RETURN ();	      
	    }

	  _rl_internal_char_cleanup ();
	}
      else if (RL_ISSTATE (RL_STATE_VIMOTION))
	{
	  eof = _rl_vi_domove_callback (_rl_vimvcxt);
	  /* Should handle everything, including cleanup, numeric arguments,
	     and turning off RL_STATE_VIMOTION */
	  if (RL_ISSTATE (RL_STATE_NUMERICARG) == 0)
	    _rl_internal_char_cleanup ();

	  CALLBACK_READ_RETURN ();
	}
#endif
      else if (RL_ISSTATE (RL_STATE_NUMERICARG))
	{
	  eof = _rl_arg_callback (_rl_argcxt);
	  if (eof == 0 && (RL_ISSTATE (RL_STATE_NUMERICARG) == 0) && RL_ISSTATE (RL_STATE_INPUTPENDING))
	    rl_callback_read_char ();
	  /* XXX - this should handle _rl_last_command_was_kill better */
	  else if (RL_ISSTATE (RL_STATE_NUMERICARG) == 0)
	    _rl_internal_char_cleanup ();

	  CALLBACK_READ_RETURN ();
	}
      else if (RL_ISSTATE (RL_STATE_MULTIKEY))
	{
	  eof = _rl_dispatch_callback (_rl_kscxt);	/* For now */
	  while ((eof == -1 || eof == -2) && RL_ISSTATE (RL_STATE_MULTIKEY) && _rl_kscxt && (_rl_kscxt->flags & KSEQ_DISPATCHED))
	    eof = _rl_dispatch_callback (_rl_kscxt);
	  if (RL_ISSTATE (RL_STATE_MULTIKEY) == 0)
	    {
	      _rl_internal_char_cleanup ();
	      _rl_want_redisplay = 1;
	    }
	}
      else if (_rl_callback_func)
	{
	  /* This allows functions that simply need to read an additional
	     character (like quoted-insert) to register a function to be
	     called when input is available.  _rl_callback_data is a
	     pointer to a struct that has the argument count originally
	     passed to the registering function and space for any additional
	     parameters.  */
	  eof = (*_rl_callback_func) (_rl_callback_data);
	  /* If the function `deregisters' itself, make sure the data is
	     cleaned up. */
	  if (_rl_callback_func == 0)
	    {
	      if (_rl_callback_data) 	
		{
		  _rl_callback_data_dispose (_rl_callback_data);
		  _rl_callback_data = 0;
		}
	      _rl_internal_char_cleanup ();
	    }
	}
      else
	eof = readline_internal_char ();

      RL_CHECK_SIGNALS ();
      if (rl_done == 0 && _rl_want_redisplay)
	{
	  (*rl_redisplay_function) ();
	  _rl_want_redisplay = 0;
	}

      if (rl_done)
	{
	  line = readline_internal_teardown (eof);

	  if (rl_deprep_term_function)
	    (*rl_deprep_term_function) ();
#if defined (HANDLE_SIGNALS)
	  rl_clear_signals ();
#endif
	  in_handler = 0;
	  (*rl_linefunc) (line);

	  /* If the user did not clear out the line, do it for him. */
	  if (rl_line_buffer[0])
	    _rl_init_line_state ();

	  /* Redisplay the prompt if readline_handler_{install,remove}
	     not called. */
	  if (in_handler == 0 && rl_linefunc)
	    _rl_callback_newline ();
	}
    }
  while (rl_pending_input || _rl_pushed_input_available () || RL_ISSTATE (RL_STATE_MACROINPUT));

  CALLBACK_READ_RETURN ();
}