void Scanner::lex_cpp_comment()
{
loop:

#line 3758 "src/parse/lex.cc"
{
	unsigned char yych;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '\n') {
		if (yych <= 0x00) goto yy566;
		if (yych <= '\t') goto yy568;
		goto yy570;
	} else {
		if (yych == '\r') goto yy572;
		goto yy568;
	}
yy566:
	++cur;
#line 732 "../src/parse/lex.re"
	{ fail_if_eof(); goto loop; }
#line 3775 "src/parse/lex.cc"
yy568:
	++cur;
yy569:
#line 733 "../src/parse/lex.re"
	{ goto loop; }
#line 3781 "src/parse/lex.cc"
yy570:
	++cur;
#line 731 "../src/parse/lex.re"
	{ next_line(); return; }
#line 3786 "src/parse/lex.cc"
yy572:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy570;
	goto yy569;
}
#line 734 "../src/parse/lex.re"

}