parse_tag(char **s, int internal)
{
    struct parsed_tag *tag = NULL;
    int tag_id;
    char tagname[MAX_TAG_LEN], attrname[MAX_TAG_LEN];
    char *p, *q;
    int i, attr_id = 0, nattr;

    /* Parse tag name */
    q = (*s) + 1;
    p = tagname;
    if (*q == '/') {
	*(p++) = *(q++);
	SKIP_BLANKS(q);
    }
    while (*q && !IS_SPACE(*q) && !(tagname[0] != '/' && *q == '/') &&
	   *q != '>' && p - tagname < MAX_TAG_LEN - 1) {
	*(p++) = TOLOWER(*q);
	q++;
    }
    *p = '\0';
    while (*q && !IS_SPACE(*q) && !(tagname[0] != '/' && *q == '/') &&
	   *q != '>')
	q++;

    tag_id = getHash_si(&tagtable, tagname, HTML_UNKNOWN);

    if (tag_id == HTML_UNKNOWN ||
	(!internal && TagMAP[tag_id].flag & TFLG_INT))
	goto skip_parse_tagarg;

    tag = New(struct parsed_tag);
    bzero(tag, sizeof(struct parsed_tag));
    tag->tagid = tag_id;

    if ((nattr = TagMAP[tag_id].max_attribute) > 0) {
	tag->attrid = NewAtom_N(unsigned char, nattr);
	tag->value = New_N(char *, nattr);
	tag->map = NewAtom_N(unsigned char, MAX_TAGATTR);
	memset(tag->map, MAX_TAGATTR, MAX_TAGATTR);
	memset(tag->attrid, ATTR_UNKNOWN, nattr);
	for (i = 0; i < nattr; i++)
	    tag->map[TagMAP[tag_id].accept_attribute[i]] = i;
    }

    /* Parse tag arguments */
    SKIP_BLANKS(q);
    while (1) {
       Str value = NULL, value_tmp = NULL;
	if (*q == '>' || *q == '\0')
	    goto done_parse_tag;
	p = attrname;
	while (*q && *q != '=' && !IS_SPACE(*q) &&
	       *q != '>' && p - attrname < MAX_TAG_LEN - 1) {
	    *(p++) = TOLOWER(*q);
	    q++;
	}
	*p = '\0';
	while (*q && *q != '=' && !IS_SPACE(*q) && *q != '>')
	    q++;
	SKIP_BLANKS(q);
	if (*q == '=') {
	    /* get value */
	    value_tmp = Strnew();
	    q++;
	    SKIP_BLANKS(q);
	    if (*q == '"') {
		q++;
		while (*q && *q != '"') {
		    Strcat_char(value_tmp, *q);
		    if (!tag->need_reconstruct && is_html_quote(*q))
			tag->need_reconstruct = TRUE;
		    q++;
		}
		if (*q == '"')
		    q++;
	    }
	    else if (*q == '\'') {
		q++;
		while (*q && *q != '\'') {
		    Strcat_char(value_tmp, *q);
		    if (!tag->need_reconstruct && is_html_quote(*q))
			tag->need_reconstruct = TRUE;
		    q++;
		}
		if (*q == '\'')
		    q++;
	    }
	    else if (*q) {
		while (*q && !IS_SPACE(*q) && *q != '>') {
                   Strcat_char(value_tmp, *q);
		    if (!tag->need_reconstruct && is_html_quote(*q))
			tag->need_reconstruct = TRUE;
		    q++;
		}
	    }
	}
	for (i = 0; i < nattr; i++) {
	    if ((tag)->attrid[i] == ATTR_UNKNOWN &&
		strcmp(AttrMAP[TagMAP[tag_id].accept_attribute[i]].name,
		       attrname) == 0) {
		attr_id = TagMAP[tag_id].accept_attribute[i];
		break;
	    }
	}

       if (value_tmp) {
         int j, hidden=FALSE;
         for (j=0; j<i; j++) {
           if (tag->attrid[j] == ATTR_TYPE &&
               tag->value[j] &&
               strcmp("hidden",tag->value[j]) == 0) {
             hidden=TRUE;
             break;
           }
         }
         if ((tag_id == HTML_INPUT || tag_id == HTML_INPUT_ALT) &&
             attr_id == ATTR_VALUE && hidden) {
           value = value_tmp;
         } else {
           char *x;
           value = Strnew();
           for (x = value_tmp->ptr; *x; x++) {
             if (*x != '\n')
               Strcat_char(value, *x);
           }
         }
       }

	if (i != nattr) {
	    if (!internal &&
		((AttrMAP[attr_id].flag & AFLG_INT) ||
		 (value && AttrMAP[attr_id].vtype == VTYPE_METHOD &&
		  !strcasecmp(value->ptr, "internal")))) {
		tag->need_reconstruct = TRUE;
		continue;
	    }
	    tag->attrid[i] = attr_id;
	    if (value)
		tag->value[i] = html_unquote(value->ptr);
	    else
		tag->value[i] = NULL;
	}
	else {
	    tag->need_reconstruct = TRUE;
	}
    }

  skip_parse_tagarg:
    while (*q != '>' && *q)
	q++;
  done_parse_tag:
    if (*q == '>')
	q++;
    *s = q;
    return tag;
}