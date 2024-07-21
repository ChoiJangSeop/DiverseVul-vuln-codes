bool Scanner::lex_block(Output &out, CodeKind kind, uint32_t indent, uint32_t mask)
{
    code_alc_t &alc = out.allocator;
    const char *fmt = NULL, *sep = NULL;
    BlockNameList *blocks;

    out.wraw(tok, ptr, !globopts->iFlag);
    if (!lex_name_list(alc, &blocks)) return false;

loop:

#line 1524 "src/parse/lex.cc"
{
	unsigned char yych;
	static const unsigned char yybm[] = {
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0, 128,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		128,   0,   0,   0,   0,   0,   0,   0, 
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
	if ((lim - cur) < 9) { if (!fill(9)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 128) {
		goto yy247;
	}
	if (yych <= ')') {
		if (yych <= '\n') {
			if (yych >= '\t') goto yy250;
		} else {
			if (yych == '\r') goto yy252;
		}
	} else {
		if (yych <= 'f') {
			if (yych <= '*') goto yy253;
			if (yych >= 'f') goto yy254;
		} else {
			if (yych == 's') goto yy255;
		}
	}
	++cur;
yy246:
#line 382 "../src/parse/lex.re"
	{
        msg.error(cur_loc(), "ill-formed directive: expected optional "
            "configurations followed by the end of block `*" "/`");
        return false;
    }
#line 1588 "src/parse/lex.cc"
yy247:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 128) {
		goto yy247;
	}
#line 406 "../src/parse/lex.re"
	{ goto loop; }
#line 1598 "src/parse/lex.cc"
yy250:
	++cur;
#line 408 "../src/parse/lex.re"
	{ next_line(); goto loop; }
#line 1603 "src/parse/lex.cc"
yy252:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy250;
	goto yy246;
yy253:
	yych = (unsigned char)*++cur;
	if (yych == '/') goto yy256;
	goto yy246;
yy254:
	yych = (unsigned char)*(mar = ++cur);
	if (yych == 'o') goto yy258;
	goto yy246;
yy255:
	yych = (unsigned char)*(mar = ++cur);
	if (yych == 'e') goto yy260;
	goto yy246;
yy256:
	++cur;
#line 410 "../src/parse/lex.re"
	{
        out.wdelay_stmt(0, code_line_info_output(alc));
        out.wdelay_stmt(indent, code_fmt(alc, kind, blocks, fmt, sep));
        out.wdelay_stmt(0, code_line_info_input(alc, cur_loc()));
        return true;
    }
#line 1629 "src/parse/lex.cc"
yy258:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy261;
yy259:
	cur = mar;
	goto yy246;
yy260:
	yych = (unsigned char)*++cur;
	if (yych == 'p') goto yy262;
	goto yy259;
yy261:
	yych = (unsigned char)*++cur;
	if (yych == 'm') goto yy263;
	goto yy259;
yy262:
	yych = (unsigned char)*++cur;
	if (yych == 'a') goto yy264;
	goto yy259;
yy263:
	yych = (unsigned char)*++cur;
	if (yych == 'a') goto yy265;
	goto yy259;
yy264:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy266;
	goto yy259;
yy265:
	yych = (unsigned char)*++cur;
	if (yych == 't') goto yy267;
	goto yy259;
yy266:
	yych = (unsigned char)*++cur;
	if (yych == 'a') goto yy269;
	goto yy259;
yy267:
	++cur;
#line 388 "../src/parse/lex.re"
	{
        if ((mask & DCONF_FORMAT) == 0) {
            msg.error(cur_loc(), "unexpected configuration 'format'");
            return false;
        }
        fmt = copystr(lex_conf_string(), alc);
        goto loop;
    }
#line 1675 "src/parse/lex.cc"
yy269:
	yych = (unsigned char)*++cur;
	if (yych != 't') goto yy259;
	yych = (unsigned char)*++cur;
	if (yych != 'o') goto yy259;
	yych = (unsigned char)*++cur;
	if (yych != 'r') goto yy259;
	++cur;
#line 397 "../src/parse/lex.re"
	{
        if ((mask & DCONF_SEPARATOR) == 0) {
            msg.error(cur_loc(), "unexpected configuration 'separator'");
            return false;
        }
        sep = copystr(lex_conf_string(), alc);
        goto loop;
    }
#line 1693 "src/parse/lex.cc"
}
#line 416 "../src/parse/lex.re"

}