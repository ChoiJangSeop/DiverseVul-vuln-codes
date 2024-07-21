m4_mkstemp (struct obstack *obs, int argc, token_data **argv)
{
  if (bad_argc (argv[0], argc, 2, 2))
    return;
  mkstemp_helper (obs, ARG (1));
}