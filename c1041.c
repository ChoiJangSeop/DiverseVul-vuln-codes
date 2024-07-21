lex (void)
{
  unsigned int c, c2;
  int backslash = 0;
  charclass ccl;
  int i;

  /* Basic plan: We fetch a character.  If it's a backslash,
     we set the backslash flag and go through the loop again.
     On the plus side, this avoids having a duplicate of the
     main switch inside the backslash case.  On the minus side,
     it means that just about every case begins with
     "if (backslash) ...".  */
  for (i = 0; i < 2; ++i)
    {
      if (MB_CUR_MAX > 1)
        {
          FETCH_WC (c, wctok, NULL);
          if ((int)c == EOF)
            goto normal_char;
        }
      else
        FETCH(c, NULL);

      switch (c)
        {
        case '\\':
          if (backslash)
            goto normal_char;
          if (lexleft == 0)
            dfaerror(_("unfinished \\ escape"));
          backslash = 1;
          break;

        case '^':
          if (backslash)
            goto normal_char;
          if (syntax_bits & RE_CONTEXT_INDEP_ANCHORS
              || lasttok == END
              || lasttok == LPAREN
              || lasttok == OR)
            return lasttok = BEGLINE;
          goto normal_char;

        case '$':
          if (backslash)
            goto normal_char;
          if (syntax_bits & RE_CONTEXT_INDEP_ANCHORS
              || lexleft == 0
              || (syntax_bits & RE_NO_BK_PARENS
                  ? lexleft > 0 && *lexptr == ')'
                  : lexleft > 1 && lexptr[0] == '\\' && lexptr[1] == ')')
              || (syntax_bits & RE_NO_BK_VBAR
                  ? lexleft > 0 && *lexptr == '|'
                  : lexleft > 1 && lexptr[0] == '\\' && lexptr[1] == '|')
              || ((syntax_bits & RE_NEWLINE_ALT)
                  && lexleft > 0 && *lexptr == '\n'))
            return lasttok = ENDLINE;
          goto normal_char;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          if (backslash && !(syntax_bits & RE_NO_BK_REFS))
            {
              laststart = 0;
              return lasttok = BACKREF;
            }
          goto normal_char;

        case '`':
          if (backslash && !(syntax_bits & RE_NO_GNU_OPS))
            return lasttok = BEGLINE;	/* FIXME: should be beginning of string */
          goto normal_char;

        case '\'':
          if (backslash && !(syntax_bits & RE_NO_GNU_OPS))
            return lasttok = ENDLINE;	/* FIXME: should be end of string */
          goto normal_char;

        case '<':
          if (backslash && !(syntax_bits & RE_NO_GNU_OPS))
            return lasttok = BEGWORD;
          goto normal_char;

        case '>':
          if (backslash && !(syntax_bits & RE_NO_GNU_OPS))
            return lasttok = ENDWORD;
          goto normal_char;

        case 'b':
          if (backslash && !(syntax_bits & RE_NO_GNU_OPS))
            return lasttok = LIMWORD;
          goto normal_char;

        case 'B':
          if (backslash && !(syntax_bits & RE_NO_GNU_OPS))
            return lasttok = NOTLIMWORD;
          goto normal_char;

        case '?':
          if (syntax_bits & RE_LIMITED_OPS)
            goto normal_char;
          if (backslash != ((syntax_bits & RE_BK_PLUS_QM) != 0))
            goto normal_char;
          if (!(syntax_bits & RE_CONTEXT_INDEP_OPS) && laststart)
            goto normal_char;
          return lasttok = QMARK;

        case '*':
          if (backslash)
            goto normal_char;
          if (!(syntax_bits & RE_CONTEXT_INDEP_OPS) && laststart)
            goto normal_char;
          return lasttok = STAR;

        case '+':
          if (syntax_bits & RE_LIMITED_OPS)
            goto normal_char;
          if (backslash != ((syntax_bits & RE_BK_PLUS_QM) != 0))
            goto normal_char;
          if (!(syntax_bits & RE_CONTEXT_INDEP_OPS) && laststart)
            goto normal_char;
          return lasttok = PLUS;

        case '{':
          if (!(syntax_bits & RE_INTERVALS))
            goto normal_char;
          if (backslash != ((syntax_bits & RE_NO_BK_BRACES) == 0))
            goto normal_char;
          if (!(syntax_bits & RE_CONTEXT_INDEP_OPS) && laststart)
            goto normal_char;

          if (syntax_bits & RE_NO_BK_BRACES)
            {
              /* Scan ahead for a valid interval; if it's not valid,
                 treat it as a literal '{'.  */
              int lo = -1, hi = -1;
              char const *p = lexptr;
              char const *lim = p + lexleft;
              for (;  p != lim && ISASCIIDIGIT (*p);  p++)
                lo = (lo < 0 ? 0 : lo * 10) + *p - '0';
              if (p != lim && *p == ',')
                while (++p != lim && ISASCIIDIGIT (*p))
                  hi = (hi < 0 ? 0 : hi * 10) + *p - '0';
              else
                hi = lo;
              if (p == lim || *p != '}'
                  || lo < 0 || RE_DUP_MAX < hi || (0 <= hi && hi < lo))
                goto normal_char;
            }

          minrep = 0;
          /* Cases:
             {M} - exact count
             {M,} - minimum count, maximum is infinity
             {M,N} - M through N */
          FETCH(c, _("unfinished repeat count"));
          if (ISASCIIDIGIT (c))
            {
              minrep = c - '0';
              for (;;)
                {
                  FETCH(c, _("unfinished repeat count"));
                  if (! ISASCIIDIGIT (c))
                    break;
                  minrep = 10 * minrep + c - '0';
                }
            }
          else
            dfaerror(_("malformed repeat count"));
          if (c == ',')
            {
              FETCH (c, _("unfinished repeat count"));
              if (! ISASCIIDIGIT (c))
                maxrep = -1;
              else
                {
                  maxrep = c - '0';
                  for (;;)
                    {
                      FETCH (c, _("unfinished repeat count"));
                      if (! ISASCIIDIGIT (c))
                        break;
                      maxrep = 10 * maxrep + c - '0';
                    }
                  if (0 <= maxrep && maxrep < minrep)
                    dfaerror (_("malformed repeat count"));
                }
            }
          else
            maxrep = minrep;
          if (!(syntax_bits & RE_NO_BK_BRACES))
            {
              if (c != '\\')
                dfaerror(_("malformed repeat count"));
              FETCH(c, _("unfinished repeat count"));
            }
          if (c != '}')
            dfaerror(_("malformed repeat count"));
          laststart = 0;
          return lasttok = REPMN;

        case '|':
          if (syntax_bits & RE_LIMITED_OPS)
            goto normal_char;
          if (backslash != ((syntax_bits & RE_NO_BK_VBAR) == 0))
            goto normal_char;
          laststart = 1;
          return lasttok = OR;

        case '\n':
          if (syntax_bits & RE_LIMITED_OPS
              || backslash
              || !(syntax_bits & RE_NEWLINE_ALT))
            goto normal_char;
          laststart = 1;
          return lasttok = OR;

        case '(':
          if (backslash != ((syntax_bits & RE_NO_BK_PARENS) == 0))
            goto normal_char;
          ++parens;
          laststart = 1;
          return lasttok = LPAREN;

        case ')':
          if (backslash != ((syntax_bits & RE_NO_BK_PARENS) == 0))
            goto normal_char;
          if (parens == 0 && syntax_bits & RE_UNMATCHED_RIGHT_PAREN_ORD)
            goto normal_char;
          --parens;
          laststart = 0;
          return lasttok = RPAREN;

        case '.':
          if (backslash)
            goto normal_char;
          if (MB_CUR_MAX > 1)
            {
              /* In multibyte environment period must match with a single
                 character not a byte.  So we use ANYCHAR.  */
              laststart = 0;
              return lasttok = ANYCHAR;
            }
          zeroset(ccl);
          notset(ccl);
          if (!(syntax_bits & RE_DOT_NEWLINE))
            clrbit(eolbyte, ccl);
          if (syntax_bits & RE_DOT_NOT_NULL)
            clrbit('\0', ccl);
          laststart = 0;
          return lasttok = CSET + charclass_index(ccl);

        case 's':
        case 'S':
          if (!backslash || (syntax_bits & RE_NO_GNU_OPS))
            goto normal_char;
          zeroset(ccl);
          for (c2 = 0; c2 < NOTCHAR; ++c2)
            if (isspace(c2))
              setbit(c2, ccl);
          if (c == 'S')
            notset(ccl);
          laststart = 0;
          return lasttok = CSET + charclass_index(ccl);

        case 'w':
        case 'W':
          if (!backslash || (syntax_bits & RE_NO_GNU_OPS))
            goto normal_char;
          zeroset(ccl);
          for (c2 = 0; c2 < NOTCHAR; ++c2)
            if (IS_WORD_CONSTITUENT(c2))
              setbit(c2, ccl);
          if (c == 'W')
            notset(ccl);
          laststart = 0;
          return lasttok = CSET + charclass_index(ccl);

        case '[':
          if (backslash)
            goto normal_char;
          laststart = 0;
          return lasttok = parse_bracket_exp();

        default:
        normal_char:
          laststart = 0;
          /* For multibyte character sets, folding is done in atom.  Always
             return WCHAR.  */
          if (MB_CUR_MAX > 1)
            return lasttok = WCHAR;

          if (case_fold && isalpha(c))
            {
              zeroset(ccl);
              setbit_case_fold_c (c, ccl);
              return lasttok = CSET + charclass_index(ccl);
            }

          return lasttok = c;
        }
    }

  /* The above loop should consume at most a backslash
     and some other character. */
  abort();
  return END;	/* keeps pedantic compilers happy. */
}