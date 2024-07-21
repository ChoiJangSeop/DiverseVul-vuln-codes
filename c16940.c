_rl_callback_newline ()
{
  rl_initialize ();

  if (in_handler == 0)
    {
      in_handler = 1;

      if (rl_prep_term_function)
	(*rl_prep_term_function) (_rl_meta_flag);
    }

  readline_internal_setup ();
  RL_CHECK_SIGNALS ();
}