get_next_token(memarea_t *area,
               const char **s, const char *eos, token_rule_t *table)
{
  /** Reject any object at least this big; it is probably an overflow, an
   * attack, a bug, or some other nonsense. */
#define MAX_UNPARSED_OBJECT_SIZE (128*1024)
  /** Reject any line at least this big; it is probably an overflow, an
   * attack, a bug, or some other nonsense. */
#define MAX_LINE_LENGTH (128*1024)

  const char *next, *eol, *obstart;
  size_t obname_len;
  int i;
  directory_token_t *tok;
  obj_syntax o_syn = NO_OBJ;
  char ebuf[128];
  const char *kwd = "";

  tor_assert(area);
  tok = ALLOC_ZERO(sizeof(directory_token_t));
  tok->tp = ERR_;

  /* Set *s to first token, eol to end-of-line, next to after first token */
  *s = eat_whitespace_eos(*s, eos); /* eat multi-line whitespace */
  tor_assert(eos >= *s);
  eol = memchr(*s, '\n', eos-*s);
  if (!eol)
    eol = eos;
  if (eol - *s > MAX_LINE_LENGTH) {
    RET_ERR("Line far too long");
  }

  next = find_whitespace_eos(*s, eol);

  if (!strcmp_len(*s, "opt", next-*s)) {
    /* Skip past an "opt" at the start of the line. */
    *s = eat_whitespace_eos_no_nl(next, eol);
    next = find_whitespace_eos(*s, eol);
  } else if (*s == eos) {  /* If no "opt", and end-of-line, line is invalid */
    RET_ERR("Unexpected EOF");
  }

  /* Search the table for the appropriate entry.  (I tried a binary search
   * instead, but it wasn't any faster.) */
  for (i = 0; table[i].t ; ++i) {
    if (!strcmp_len(*s, table[i].t, next-*s)) {
      /* We've found the keyword. */
      kwd = table[i].t;
      tok->tp = table[i].v;
      o_syn = table[i].os;
      *s = eat_whitespace_eos_no_nl(next, eol);
      /* We go ahead whether there are arguments or not, so that tok->args is
       * always set if we want arguments. */
      if (table[i].concat_args) {
        /* The keyword takes the line as a single argument */
        tok->args = ALLOC(sizeof(char*));
        tok->args[0] = STRNDUP(*s,eol-*s); /* Grab everything on line */
        tok->n_args = 1;
      } else {
        /* This keyword takes multiple arguments. */
        if (get_token_arguments(area, tok, *s, eol)<0) {
          tor_snprintf(ebuf, sizeof(ebuf),"Far too many arguments to %s", kwd);
          RET_ERR(ebuf);
        }
        *s = eol;
      }
      if (tok->n_args < table[i].min_args) {
        tor_snprintf(ebuf, sizeof(ebuf), "Too few arguments to %s", kwd);
        RET_ERR(ebuf);
      } else if (tok->n_args > table[i].max_args) {
        tor_snprintf(ebuf, sizeof(ebuf), "Too many arguments to %s", kwd);
        RET_ERR(ebuf);
      }
      break;
    }
  }

  if (tok->tp == ERR_) {
    /* No keyword matched; call it an "K_opt" or "A_unrecognized" */
    if (**s == '@')
      tok->tp = A_UNKNOWN_;
    else
      tok->tp = K_OPT;
    tok->args = ALLOC(sizeof(char*));
    tok->args[0] = STRNDUP(*s, eol-*s);
    tok->n_args = 1;
    o_syn = OBJ_OK;
  }

  /* Check whether there's an object present */
  *s = eat_whitespace_eos(eol, eos);  /* Scan from end of first line */
  tor_assert(eos >= *s);
  eol = memchr(*s, '\n', eos-*s);
  if (!eol || eol-*s<11 || strcmpstart(*s, "-----BEGIN ")) /* No object. */
    goto check_object;

  obstart = *s; /* Set obstart to start of object spec */
  if (*s+16 >= eol || memchr(*s+11,'\0',eol-*s-16) || /* no short lines, */
      strcmp_len(eol-5, "-----", 5) ||           /* nuls or invalid endings */
      (eol-*s) > MAX_UNPARSED_OBJECT_SIZE) {     /* name too long */
    RET_ERR("Malformed object: bad begin line");
  }
  tok->object_type = STRNDUP(*s+11, eol-*s-16);
  obname_len = eol-*s-16; /* store objname length here to avoid a strlen() */
  *s = eol+1;    /* Set *s to possible start of object data (could be eos) */

  /* Go to the end of the object */
  next = tor_memstr(*s, eos-*s, "-----END ");
  if (!next) {
    RET_ERR("Malformed object: missing object end line");
  }
  tor_assert(eos >= next);
  eol = memchr(next, '\n', eos-next);
  if (!eol)  /* end-of-line marker, or eos if there's no '\n' */
    eol = eos;
  /* Validate the ending tag, which should be 9 + NAME + 5 + eol */
  if ((size_t)(eol-next) != 9+obname_len+5 ||
      strcmp_len(next+9, tok->object_type, obname_len) ||
      strcmp_len(eol-5, "-----", 5)) {
    tor_snprintf(ebuf, sizeof(ebuf), "Malformed object: mismatched end tag %s",
             tok->object_type);
    ebuf[sizeof(ebuf)-1] = '\0';
    RET_ERR(ebuf);
  }
  if (next - *s > MAX_UNPARSED_OBJECT_SIZE)
    RET_ERR("Couldn't parse object: missing footer or object much too big.");

  if (!strcmp(tok->object_type, "RSA PUBLIC KEY")) { /* If it's a public key */
    tok->key = crypto_pk_new();
    if (crypto_pk_read_public_key_from_string(tok->key, obstart, eol-obstart))
      RET_ERR("Couldn't parse public key.");
  } else if (!strcmp(tok->object_type, "RSA PRIVATE KEY")) { /* private key */
    tok->key = crypto_pk_new();
    if (crypto_pk_read_private_key_from_string(tok->key, obstart, eol-obstart))
      RET_ERR("Couldn't parse private key.");
  } else { /* If it's something else, try to base64-decode it */
    int r;
    tok->object_body = ALLOC(next-*s); /* really, this is too much RAM. */
    r = base64_decode(tok->object_body, next-*s, *s, next-*s);
    if (r<0)
      RET_ERR("Malformed object: bad base64-encoded data");
    tok->object_size = r;
  }
  *s = eol;

 check_object:
  tok = token_check_object(area, kwd, tok, o_syn);

 done_tokenizing:
  return tok;

#undef RET_ERR
#undef ALLOC
#undef ALLOC_ZERO
#undef STRDUP
#undef STRNDUP
}