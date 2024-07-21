void Scanner::lex_c_comment()
{
loop:

#line 3708 "src/parse/lex.cc"
{
	unsigned char yych;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '\f') {
		if (yych <= 0x00) goto yy554;
		if (yych == '\n') goto yy558;
		goto yy556;
	} else {
		if (yych <= '\r') goto yy560;
		if (yych == '*') goto yy561;
		goto yy556;
	}
yy554:
	++cur;
#line 722 "../src/parse/lex.re"
	{ fail_if_eof(); goto loop; }
#line 3726 "src/parse/lex.cc"
yy556:
	++cur;
yy557:
#line 723 "../src/parse/lex.re"
	{ goto loop; }
#line 3732 "src/parse/lex.cc"
yy558:
	++cur;
#line 721 "../src/parse/lex.re"
	{ next_line(); goto loop; }
#line 3737 "src/parse/lex.cc"
yy560:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy558;
	goto yy557;
yy561:
	yych = (unsigned char)*++cur;
	if (yych != '/') goto yy557;
	++cur;
#line 720 "../src/parse/lex.re"
	{ return; }
#line 3748 "src/parse/lex.cc"
}
#line 724 "../src/parse/lex.re"

}