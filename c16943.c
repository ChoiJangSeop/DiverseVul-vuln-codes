dump_word_flags (flags)
     int flags;
{
  int f;

  f = flags;
  fprintf (stderr, "%d -> ", f);
  if (f & W_ARRAYIND)
    {
      f &= ~W_ARRAYIND;
      fprintf (stderr, "W_ARRAYIND%s", f ? "|" : "");
    }
  if (f & W_ASSIGNASSOC)
    {
      f &= ~W_ASSIGNASSOC;
      fprintf (stderr, "W_ASSIGNASSOC%s", f ? "|" : "");
    }
  if (f & W_ASSIGNARRAY)
    {
      f &= ~W_ASSIGNARRAY;
      fprintf (stderr, "W_ASSIGNARRAY%s", f ? "|" : "");
    }
  if (f & W_HASCTLESC)
    {
      f &= ~W_HASCTLESC;
      fprintf (stderr, "W_HASCTLESC%s", f ? "|" : "");
    }
  if (f & W_NOPROCSUB)
    {
      f &= ~W_NOPROCSUB;
      fprintf (stderr, "W_NOPROCSUB%s", f ? "|" : "");
    }
  if (f & W_DQUOTE)
    {
      f &= ~W_DQUOTE;
      fprintf (stderr, "W_DQUOTE%s", f ? "|" : "");
    }
  if (f & W_HASQUOTEDNULL)
    {
      f &= ~W_HASQUOTEDNULL;
      fprintf (stderr, "W_HASQUOTEDNULL%s", f ? "|" : "");
    }
  if (f & W_ASSIGNARG)
    {
      f &= ~W_ASSIGNARG;
      fprintf (stderr, "W_ASSIGNARG%s", f ? "|" : "");
    }
  if (f & W_ASSNBLTIN)
    {
      f &= ~W_ASSNBLTIN;
      fprintf (stderr, "W_ASSNBLTIN%s", f ? "|" : "");
    }
  if (f & W_ASSNGLOBAL)
    {
      f &= ~W_ASSNGLOBAL;
      fprintf (stderr, "W_ASSNGLOBAL%s", f ? "|" : "");
    }
  if (f & W_COMPASSIGN)
    {
      f &= ~W_COMPASSIGN;
      fprintf (stderr, "W_COMPASSIGN%s", f ? "|" : "");
    }
  if (f & W_NOEXPAND)
    {
      f &= ~W_NOEXPAND;
      fprintf (stderr, "W_NOEXPAND%s", f ? "|" : "");
    }
  if (f & W_ITILDE)
    {
      f &= ~W_ITILDE;
      fprintf (stderr, "W_ITILDE%s", f ? "|" : "");
    }
  if (f & W_NOTILDE)
    {
      f &= ~W_NOTILDE;
      fprintf (stderr, "W_NOTILDE%s", f ? "|" : "");
    }
  if (f & W_ASSIGNRHS)
    {
      f &= ~W_ASSIGNRHS;
      fprintf (stderr, "W_ASSIGNRHS%s", f ? "|" : "");
    }
  if (f & W_NOCOMSUB)
    {
      f &= ~W_NOCOMSUB;
      fprintf (stderr, "W_NOCOMSUB%s", f ? "|" : "");
    }
  if (f & W_DOLLARSTAR)
    {
      f &= ~W_DOLLARSTAR;
      fprintf (stderr, "W_DOLLARSTAR%s", f ? "|" : "");
    }
  if (f & W_DOLLARAT)
    {
      f &= ~W_DOLLARAT;
      fprintf (stderr, "W_DOLLARAT%s", f ? "|" : "");
    }
  if (f & W_TILDEEXP)
    {
      f &= ~W_TILDEEXP;
      fprintf (stderr, "W_TILDEEXP%s", f ? "|" : "");
    }
  if (f & W_NOSPLIT2)
    {
      f &= ~W_NOSPLIT2;
      fprintf (stderr, "W_NOSPLIT2%s", f ? "|" : "");
    }
  if (f & W_NOSPLIT)
    {
      f &= ~W_NOSPLIT;
      fprintf (stderr, "W_NOSPLIT%s", f ? "|" : "");
    }
  if (f & W_NOBRACE)
    {
      f &= ~W_NOBRACE;
      fprintf (stderr, "W_NOBRACE%s", f ? "|" : "");
    }
  if (f & W_NOGLOB)
    {
      f &= ~W_NOGLOB;
      fprintf (stderr, "W_NOGLOB%s", f ? "|" : "");
    }
  if (f & W_SPLITSPACE)
    {
      f &= ~W_SPLITSPACE;
      fprintf (stderr, "W_SPLITSPACE%s", f ? "|" : "");
    }
  if (f & W_ASSIGNMENT)
    {
      f &= ~W_ASSIGNMENT;
      fprintf (stderr, "W_ASSIGNMENT%s", f ? "|" : "");
    }
  if (f & W_QUOTED)
    {
      f &= ~W_QUOTED;
      fprintf (stderr, "W_QUOTED%s", f ? "|" : "");
    }
  if (f & W_HASDOLLAR)
    {
      f &= ~W_HASDOLLAR;
      fprintf (stderr, "W_HASDOLLAR%s", f ? "|" : "");
    }
  fprintf (stderr, "\n");
  fflush (stderr);
}