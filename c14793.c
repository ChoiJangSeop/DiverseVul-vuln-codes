void Scanner::lex_string(char delim)
{
loop:

#line 3637 "src/parse/lex.cc"
{
	unsigned char yych;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '!') {
		if (yych <= '\n') {
			if (yych <= 0x00) goto yy540;
			if (yych <= '\t') goto yy542;
			goto yy544;
		} else {
			if (yych == '\r') goto yy546;
			goto yy542;
		}
	} else {
		if (yych <= '\'') {
			if (yych <= '"') goto yy547;
			if (yych <= '&') goto yy542;
			goto yy547;
		} else {
			if (yych == '\\') goto yy549;
			goto yy542;
		}
	}
yy540:
	++cur;
#line 711 "../src/parse/lex.re"
	{ fail_if_eof(); goto loop; }
#line 3665 "src/parse/lex.cc"
yy542:
	++cur;
yy543:
#line 712 "../src/parse/lex.re"
	{ goto loop; }
#line 3671 "src/parse/lex.cc"
yy544:
	++cur;
#line 710 "../src/parse/lex.re"
	{ next_line(); goto loop; }
#line 3676 "src/parse/lex.cc"
yy546:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy544;
	goto yy543;
yy547:
	++cur;
#line 708 "../src/parse/lex.re"
	{ if (cur[-1] == delim) return; else goto loop; }
#line 3685 "src/parse/lex.cc"
yy549:
	yych = (unsigned char)*++cur;
	if (yych <= '&') {
		if (yych != '"') goto yy543;
	} else {
		if (yych <= '\'') goto yy550;
		if (yych != '\\') goto yy543;
	}
yy550:
	++cur;
#line 709 "../src/parse/lex.re"
	{ goto loop; }
#line 3698 "src/parse/lex.cc"
}
#line 713 "../src/parse/lex.re"

}