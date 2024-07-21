_rl_get_string_variable_value (name)
     const char *name;
{
  static char numbuf[32];
  char *ret;

  if (_rl_stricmp (name, "bell-style") == 0)
    {
      switch (_rl_bell_preference)
	{
	  case NO_BELL:
	    return "none";
	  case VISIBLE_BELL:
	    return "visible";
	  case AUDIBLE_BELL:
	  default:
	    return "audible";
	}
    }
  else if (_rl_stricmp (name, "comment-begin") == 0)
    return (_rl_comment_begin ? _rl_comment_begin : RL_COMMENT_BEGIN_DEFAULT);
  else if (_rl_stricmp (name, "completion-display-width") == 0)
    {
      sprintf (numbuf, "%d", _rl_completion_columns);
      return (numbuf);
    }
  else if (_rl_stricmp (name, "completion-prefix-display-length") == 0)
    {
      sprintf (numbuf, "%d", _rl_completion_prefix_display_length);
      return (numbuf);
    }
  else if (_rl_stricmp (name, "completion-query-items") == 0)
    {
      sprintf (numbuf, "%d", rl_completion_query_items);
      return (numbuf);
    }
  else if (_rl_stricmp (name, "editing-mode") == 0)
    return (rl_get_keymap_name_from_edit_mode ());
  else if (_rl_stricmp (name, "history-size") == 0)
    {
      sprintf (numbuf, "%d", history_is_stifled() ? history_max_entries : 0);
      return (numbuf);
    }
  else if (_rl_stricmp (name, "isearch-terminators") == 0)
    {
      if (_rl_isearch_terminators == 0)
	return 0;
      ret = _rl_untranslate_macro_value (_rl_isearch_terminators, 0);
      if (ret)
	{
	  strncpy (numbuf, ret, sizeof (numbuf) - 1);
	  xfree (ret);
	  numbuf[sizeof(numbuf) - 1] = '\0';
	}
      else
	numbuf[0] = '\0';
      return numbuf;
    }
  else if (_rl_stricmp (name, "keymap") == 0)
    {
      ret = rl_get_keymap_name (_rl_keymap);
      if (ret == 0)
	ret = rl_get_keymap_name_from_edit_mode ();
      return (ret ? ret : "none");
    }
  else if (_rl_stricmp (name, "keyseq-timeout") == 0)
    {
      sprintf (numbuf, "%d", _rl_keyseq_timeout);    
      return (numbuf);
    }
  else if (_rl_stricmp (name, "emacs-mode-string") == 0)
    return (_rl_emacs_mode_str ? _rl_emacs_mode_str : RL_EMACS_MODESTR_DEFAULT);
  else if (_rl_stricmp (name, "vi-cmd-mode-string") == 0)
    return (_rl_emacs_mode_str ? _rl_emacs_mode_str : RL_VI_CMD_MODESTR_DEFAULT);
  else if (_rl_stricmp (name, "vi-ins-mode-string") == 0)
    return (_rl_emacs_mode_str ? _rl_emacs_mode_str : RL_VI_INS_MODESTR_DEFAULT);
  else
    return (0);
}