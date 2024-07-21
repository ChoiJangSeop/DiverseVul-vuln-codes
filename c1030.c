prtok (token t)
{
  char const *s;

  if (t < 0)
    fprintf(stderr, "END");
  else if (t < NOTCHAR)
    fprintf(stderr, "%c", t);
  else
    {
      switch (t)
        {
        case EMPTY: s = "EMPTY"; break;
        case BACKREF: s = "BACKREF"; break;
        case BEGLINE: s = "BEGLINE"; break;
        case ENDLINE: s = "ENDLINE"; break;
        case BEGWORD: s = "BEGWORD"; break;
        case ENDWORD: s = "ENDWORD"; break;
        case LIMWORD: s = "LIMWORD"; break;
        case NOTLIMWORD: s = "NOTLIMWORD"; break;
        case QMARK: s = "QMARK"; break;
        case STAR: s = "STAR"; break;
        case PLUS: s = "PLUS"; break;
        case CAT: s = "CAT"; break;
        case OR: s = "OR"; break;
        case LPAREN: s = "LPAREN"; break;
        case RPAREN: s = "RPAREN"; break;
        case ANYCHAR: s = "ANYCHAR"; break;
        case MBCSET: s = "MBCSET"; break;
        default: s = "CSET"; break;
        }
      fprintf(stderr, "%s", s);
    }
}