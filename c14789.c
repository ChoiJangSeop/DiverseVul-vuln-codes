bool Scanner::lex_name_list(code_alc_t &alc, BlockNameList **ptail)
{
    BlockNameList **phead = ptail;
loop:
    tok = cur;

#line 1285 "src/parse/lex.cc"
{
	unsigned char yych;
	static const unsigned char yybm[] = {
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128,   0,   0,   0,   0,   0,   0, 
		  0, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128,   0,   0,   0,   0, 128, 
		  0, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*(mar = cur);
	if (yych <= 0x1F) {
		if (yych <= '\n') {
			if (yych >= '\t') {
				yyt1 = cur;
				goto yy222;
			}
		} else {
			if (yych == '\r') {
				yyt1 = cur;
				goto yy222;
			}
		}
	} else {
		if (yych <= '*') {
			if (yych <= ' ') {
				yyt1 = cur;
				goto yy222;
			}
			if (yych >= '*') {
				yyt1 = cur;
				goto yy224;
			}
		} else {
			if (yych == ':') goto yy226;
		}
	}
yy221:
#line 319 "../src/parse/lex.re"
	{
        msg.error(cur_loc(), "ill-formed start of a block: expected a space, a"
            " newline, a colon followed by a list of colon-separated block"
            " names, or the end of block `*" "/`");
        return false;
    }
#line 1358 "src/parse/lex.cc"
yy222:
	++cur;
	cur = yyt1;
#line 326 "../src/parse/lex.re"
	{
        *ptail = NULL;
        return true;
    }
#line 1367 "src/parse/lex.cc"
yy224:
	yych = (unsigned char)*++cur;
	if (yych == '/') goto yy222;
yy225:
	cur = mar;
	goto yy221;
yy226:
	yych = (unsigned char)*++cur;
	if (yych <= '^') {
		if (yych <= '@') goto yy225;
		if (yych >= '[') goto yy225;
	} else {
		if (yych == '`') goto yy225;
		if (yych >= '{') goto yy225;
	}
yy227:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 128) {
		goto yy227;
	}
#line 331 "../src/parse/lex.re"
	{
        BlockNameList *l = alc.alloct<BlockNameList>(1);
        l->name = newcstr(tok + 1, cur, alc);
        l->next = NULL;
        *ptail = l;
        ptail = &l->next;

        // Check that the added name is unique.
        for (const BlockNameList *p = *phead; p != l; p = p->next) {
            if (strcmp(p->name, l->name) == 0) {
                msg.error(cur_loc(), "duplicate block '%s' on the list", p->name);
                return false;
            }
        }

        goto loop;
    }
#line 1408 "src/parse/lex.cc"
}
#line 348 "../src/parse/lex.re"

}