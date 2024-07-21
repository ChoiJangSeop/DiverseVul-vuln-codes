void Scanner::try_lex_string_in_code(char quote)
{
    // We need to lex string literals in code blocks because they may contain closing
    // brace symbol that would otherwise be erroneously lexed as a real closing brace.
    //
    // However, single quote in Rust may be either the beginning of a char literal as in
    // '\u{1F600}', or a standalone one as in 'label. In the latter case trying to lex a
    // generic string literal will consume a fragment of the file until the next single
    // quote (if any) and result in either a spurios parse error, or incorrect generated
    // code. Therefore in Rust we try to lex a char literal, or else consume the quote.

    if (globopts->lang != LANG_RUST || quote != '\'') {
        lex_string(quote);
        return;
    }

    // Rust spec (literals): https://doc.rust-lang.org/reference/tokens.html#literals
    // Rust spec (input encoding): https://doc.rust-lang.org/reference/input-format.html

#line 3452 "src/parse/lex.cc"
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
		  0, 128, 128, 128, 128, 128, 128,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0,   0,   0,   0,   0,   0,   0,   0, 
		  0, 128, 128, 128, 128, 128, 128,   0, 
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
	if ((lim - cur) < 5) { if (!fill(5)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*(mar = cur);
	if (yych <= 0xDF) {
		if (yych <= '\\') {
			if (yych <= '[') goto yy520;
			goto yy522;
		} else {
			if (yych <= 0x7F) goto yy520;
			if (yych >= 0xC2) goto yy523;
		}
	} else {
		if (yych <= 0xF0) {
			if (yych <= 0xE0) goto yy524;
			if (yych <= 0xEF) goto yy525;
			goto yy526;
		} else {
			if (yych <= 0xF3) goto yy527;
			if (yych <= 0xF4) goto yy528;
		}
	}
yy519:
#line 700 "../src/parse/lex.re"
	{ return; }
#line 3512 "src/parse/lex.cc"
yy520:
	yych = (unsigned char)*++cur;
	if (yych == '\'') goto yy529;
yy521:
	cur = mar;
	goto yy519;
yy522:
	yych = (unsigned char)*++cur;
	if (yych <= 'm') {
		if (yych <= '\'') {
			if (yych == '"') goto yy520;
			if (yych <= '&') goto yy521;
			goto yy530;
		} else {
			if (yych <= '0') {
				if (yych <= '/') goto yy521;
				goto yy520;
			} else {
				if (yych == '\\') goto yy520;
				goto yy521;
			}
		}
	} else {
		if (yych <= 's') {
			if (yych <= 'n') goto yy520;
			if (yych == 'r') goto yy520;
			goto yy521;
		} else {
			if (yych <= 'u') {
				if (yych <= 't') goto yy520;
				goto yy531;
			} else {
				if (yych == 'x') goto yy532;
				goto yy521;
			}
		}
	}
yy523:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy521;
	if (yych <= 0xBF) goto yy520;
	goto yy521;
yy524:
	yych = (unsigned char)*++cur;
	if (yych <= 0x9F) goto yy521;
	if (yych <= 0xBF) goto yy523;
	goto yy521;
yy525:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy521;
	if (yych <= 0xBF) goto yy523;
	goto yy521;
yy526:
	yych = (unsigned char)*++cur;
	if (yych <= 0x8F) goto yy521;
	if (yych <= 0xBF) goto yy525;
	goto yy521;
yy527:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy521;
	if (yych <= 0xBF) goto yy525;
	goto yy521;
yy528:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy521;
	if (yych <= 0x8F) goto yy525;
	goto yy521;
yy529:
	++cur;
	goto yy519;
yy530:
	yych = (unsigned char)*++cur;
	if (yych == '\'') goto yy529;
	goto yy519;
yy531:
	yych = (unsigned char)*++cur;
	if (yych == '{') goto yy533;
	goto yy521;
yy532:
	yych = (unsigned char)*++cur;
	if (yych == '\'') goto yy521;
	goto yy535;
yy533:
	yych = (unsigned char)*++cur;
	if (yych == '}') goto yy521;
	goto yy537;
yy534:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
yy535:
	if (yybm[0+yych] & 128) {
		goto yy534;
	}
	if (yych == '\'') goto yy529;
	goto yy521;
yy536:
	++cur;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
yy537:
	if (yych <= 'F') {
		if (yych <= '/') goto yy521;
		if (yych <= '9') goto yy536;
		if (yych <= '@') goto yy521;
		goto yy536;
	} else {
		if (yych <= 'f') {
			if (yych <= '`') goto yy521;
			goto yy536;
		} else {
			if (yych == '}') goto yy520;
			goto yy521;
		}
	}
}
#line 701 "../src/parse/lex.re"

}