bool Scanner::lex_namedef_context_flex()
{

#line 2739 "src/parse/lex.cc"
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
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych == '\t') {
		yyt1 = cur;
		goto yy416;
	}
	if (yych == ' ') {
		yyt1 = cur;
		goto yy416;
	}
#line 589 "../src/parse/lex.re"
	{ return false; }
#line 2788 "src/parse/lex.cc"
yy416:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 128) {
		goto yy416;
	}
	if (yych <= '<') {
		if (yych == ':') goto yy419;
	} else {
		if (yych <= '=') goto yy419;
		if (yych == '{') goto yy419;
	}
	cur = yyt1;
#line 588 "../src/parse/lex.re"
	{ return true; }
#line 2805 "src/parse/lex.cc"
yy419:
	++cur;
	cur = yyt1;
#line 587 "../src/parse/lex.re"
	{ return false; }
#line 2811 "src/parse/lex.cc"
}
#line 590 "../src/parse/lex.re"

}