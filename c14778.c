void Scanner::lex_code_indented() {
    const loc_t &loc = tok_loc();
    tok = cur;
code:

#line 3086 "src/parse/lex.cc"
{
	unsigned char yych;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '&') {
		if (yych <= '\f') {
			if (yych <= 0x00) goto yy454;
			if (yych == '\n') goto yy458;
			goto yy456;
		} else {
			if (yych <= '\r') goto yy460;
			if (yych == '"') goto yy461;
			goto yy456;
		}
	} else {
		if (yych <= 'z') {
			if (yych <= '\'') goto yy461;
			if (yych == '/') goto yy463;
			goto yy456;
		} else {
			if (yych == '|') goto yy456;
			if (yych <= '}') goto yy464;
			goto yy456;
		}
	}
yy454:
	++cur;
#line 629 "../src/parse/lex.re"
	{ fail_if_eof(); goto code; }
#line 3116 "src/parse/lex.cc"
yy456:
	++cur;
yy457:
#line 635 "../src/parse/lex.re"
	{ goto code; }
#line 3122 "src/parse/lex.cc"
yy458:
	++cur;
#line 630 "../src/parse/lex.re"
	{ next_line(); goto indent; }
#line 3127 "src/parse/lex.cc"
yy460:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy458;
	goto yy457;
yy461:
	++cur;
#line 633 "../src/parse/lex.re"
	{ try_lex_string_in_code(cur[-1]); goto code; }
#line 3136 "src/parse/lex.cc"
yy463:
	yych = (unsigned char)*++cur;
	if (yych == '*') goto yy466;
	if (yych == '/') goto yy468;
	goto yy457;
yy464:
	++cur;
#line 634 "../src/parse/lex.re"
	{ msg.error(cur_loc(), "Curly braces are not allowed after ':='"); exit(1); }
#line 3146 "src/parse/lex.cc"
yy466:
	++cur;
#line 632 "../src/parse/lex.re"
	{ lex_c_comment(); goto code; }
#line 3151 "src/parse/lex.cc"
yy468:
	++cur;
#line 631 "../src/parse/lex.re"
	{ lex_cpp_comment(); goto indent; }
#line 3156 "src/parse/lex.cc"
}
#line 636 "../src/parse/lex.re"

indent:

#line 3162 "src/parse/lex.cc"
{
	unsigned char yych;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '\f') {
		if (yych <= 0x08) goto yy472;
		if (yych <= '\n') goto yy473;
	} else {
		if (yych <= '\r') goto yy473;
		if (yych == ' ') goto yy473;
	}
yy472:
#line 640 "../src/parse/lex.re"
	{
        while (isspace(tok[0])) ++tok;
        char *p = cur;
        while (p > tok && isspace(p[-1])) --p;
        yylval.semact = new SemAct(loc, getstr(tok, p));
        return;
    }
#line 3183 "src/parse/lex.cc"
yy473:
	++cur;
	cur -= 1;
#line 639 "../src/parse/lex.re"
	{ goto code; }
#line 3189 "src/parse/lex.cc"
}
#line 647 "../src/parse/lex.re"

}