void Scanner::lex_code_in_braces()
{
    const loc_t &loc = tok_loc();
    uint32_t depth = 1;
code:

#line 3201 "src/parse/lex.cc"
{
	unsigned char yych;
	static const unsigned char yybm[] = {
		  0, 128, 128, 128, 128, 128, 128, 128, 
		128, 160,   0, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		160, 128,   0, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		192, 192, 192, 192, 192, 192, 192, 192, 
		192, 192, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128,   0, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
	};
	if ((lim - cur) < 3) { if (!fill(3)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '&') {
		if (yych <= '\f') {
			if (yych <= 0x00) goto yy477;
			if (yych == '\n') goto yy481;
			goto yy479;
		} else {
			if (yych <= '\r') goto yy483;
			if (yych == '"') goto yy484;
			goto yy479;
		}
	} else {
		if (yych <= 'z') {
			if (yych <= '\'') goto yy484;
			if (yych == '/') goto yy486;
			goto yy479;
		} else {
			if (yych <= '{') goto yy487;
			if (yych == '}') goto yy489;
			goto yy479;
		}
	}
yy477:
	++cur;
#line 667 "../src/parse/lex.re"
	{ fail_if_eof(); goto code; }
#line 3265 "src/parse/lex.cc"
yy479:
	++cur;
yy480:
#line 671 "../src/parse/lex.re"
	{ goto code; }
#line 3271 "src/parse/lex.cc"
yy481:
	yych = (unsigned char)*(mar = ++cur);
	if (yybm[0+yych] & 32) {
		goto yy491;
	}
	if (yych == '#') goto yy494;
yy482:
#line 666 "../src/parse/lex.re"
	{ next_line(); goto code; }
#line 3281 "src/parse/lex.cc"
yy483:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy481;
	goto yy480;
yy484:
	++cur;
#line 670 "../src/parse/lex.re"
	{ try_lex_string_in_code(cur[-1]); goto code; }
#line 3290 "src/parse/lex.cc"
yy486:
	yych = (unsigned char)*++cur;
	if (yych == '*') goto yy496;
	if (yych == '/') goto yy498;
	goto yy480;
yy487:
	++cur;
#line 664 "../src/parse/lex.re"
	{ ++depth; goto code; }
#line 3300 "src/parse/lex.cc"
yy489:
	++cur;
#line 656 "../src/parse/lex.re"
	{
        if (--depth == 0) {
            yylval.semact = new SemAct(loc, getstr(tok, cur));
            return;
        }
        goto code;
    }
#line 3311 "src/parse/lex.cc"
yy491:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 32) {
		goto yy491;
	}
	if (yych == '#') goto yy494;
yy493:
	cur = mar;
	goto yy482;
yy494:
	++cur;
	if ((lim - cur) < 5) { if (!fill(5)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy494;
		goto yy493;
	} else {
		if (yych <= ' ') goto yy494;
		if (yych == 'l') goto yy500;
		goto yy493;
	}
yy496:
	++cur;
#line 668 "../src/parse/lex.re"
	{ lex_c_comment(); goto code; }
#line 3339 "src/parse/lex.cc"
yy498:
	++cur;
#line 669 "../src/parse/lex.re"
	{ lex_cpp_comment(); goto code; }
#line 3344 "src/parse/lex.cc"
yy500:
	yych = (unsigned char)*++cur;
	if (yych != 'i') goto yy493;
	yych = (unsigned char)*++cur;
	if (yych != 'n') goto yy493;
	yych = (unsigned char)*++cur;
	if (yych != 'e') goto yy493;
	yych = (unsigned char)*++cur;
	if (yych <= '0') goto yy505;
	if (yych <= '9') goto yy493;
	goto yy505;
yy504:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
yy505:
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy504;
		goto yy493;
	} else {
		if (yych <= ' ') goto yy504;
		if (yych <= '0') goto yy493;
		if (yych >= ':') goto yy493;
		yyt1 = cur;
	}
yy506:
	++cur;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 64) {
		goto yy506;
	}
	if (yych <= '\f') {
		if (yych <= 0x08) goto yy493;
		if (yych <= '\t') goto yy508;
		if (yych <= '\n') goto yy510;
		goto yy493;
	} else {
		if (yych <= '\r') goto yy512;
		if (yych != ' ') goto yy493;
	}
yy508:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy508;
		goto yy493;
	} else {
		if (yych <= ' ') goto yy508;
		if (yych == '"') goto yy513;
		goto yy493;
	}
yy510:
	++cur;
	cur = yyt1;
#line 665 "../src/parse/lex.re"
	{ set_sourceline (); goto code; }
#line 3403 "src/parse/lex.cc"
yy512:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy510;
	goto yy493;
yy513:
	++cur;
	if ((lim - cur) < 3) { if (!fill(3)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 128) {
		goto yy513;
	}
	if (yych <= '\n') goto yy493;
	if (yych >= '#') goto yy516;
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy510;
	if (yych == '\r') goto yy512;
	goto yy493;
yy516:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x00) goto yy493;
	if (yych == '\n') goto yy493;
	goto yy513;
}
#line 672 "../src/parse/lex.re"

}