void Scanner::set_sourceline ()
{
sourceline:
    tok = cur;

#line 5401 "src/parse/lex.cc"
{
	unsigned char yych;
	static const unsigned char yybm[] = {
		  0, 128, 128, 128, 128, 128, 128, 128, 
		128, 128,   0, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128,   0, 128, 128, 128, 128, 128, 
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
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '\r') {
		if (yych <= '\t') {
			if (yych >= 0x01) goto yy829;
		} else {
			if (yych <= '\n') goto yy831;
			if (yych <= '\f') goto yy829;
			goto yy833;
		}
	} else {
		if (yych <= '"') {
			if (yych <= '!') goto yy829;
			goto yy834;
		} else {
			if (yych <= '0') goto yy829;
			if (yych <= '9') goto yy835;
			goto yy829;
		}
	}
	++cur;
#line 884 "../src/parse/lex.re"
	{ --cur; return; }
#line 5461 "src/parse/lex.cc"
yy829:
	++cur;
yy830:
#line 885 "../src/parse/lex.re"
	{ goto sourceline; }
#line 5467 "src/parse/lex.cc"
yy831:
	++cur;
#line 883 "../src/parse/lex.re"
	{ pos = tok = cur; return; }
#line 5472 "src/parse/lex.cc"
yy833:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy831;
	goto yy830;
yy834:
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x00) goto yy830;
	if (yych == '\n') goto yy830;
	goto yy839;
yy835:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 64) {
		goto yy835;
	}
#line 864 "../src/parse/lex.re"
	{
        uint32_t l;
        if (!s_to_u32_unsafe(tok, cur, l)) {
            msg.error(tok_loc(), "line number overflow");
            exit(1);
        }
        set_line(l);
        goto sourceline;
    }
#line 5499 "src/parse/lex.cc"
yy838:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
yy839:
	if (yybm[0+yych] & 128) {
		goto yy838;
	}
	if (yych <= '\n') goto yy840;
	if (yych <= '"') goto yy841;
	goto yy843;
yy840:
	cur = mar;
	goto yy830;
yy841:
	++cur;
#line 874 "../src/parse/lex.re"
	{
        Input &in = get_input();
        std::string &name = in.escaped_name;
        name = escape_backslashes(getstr(tok + 1, cur - 1));
        in.fidx = static_cast<uint32_t>(msg.filenames.size());
        msg.filenames.push_back(name);
        goto sourceline;
    }
#line 5525 "src/parse/lex.cc"
yy843:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x00) goto yy840;
	if (yych == '\n') goto yy840;
	goto yy838;
}
#line 886 "../src/parse/lex.re"

}