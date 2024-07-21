bool Scanner::lex_opt_name(std::string &name)
{
    tok = cur;

#line 1142 "src/parse/lex.cc"
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
				goto yy209;
			}
		} else {
			if (yych == '\r') {
				yyt1 = cur;
				goto yy209;
			}
		}
	} else {
		if (yych <= '*') {
			if (yych <= ' ') {
				yyt1 = cur;
				goto yy209;
			}
			if (yych >= '*') {
				yyt1 = cur;
				goto yy211;
			}
		} else {
			if (yych == ':') goto yy213;
		}
	}
yy208:
#line 301 "../src/parse/lex.re"
	{
        msg.error(cur_loc(), "ill-formed start of a block: expected a space, a"
            " newline, a colon followed by a block name, or the end of block `*"
            "/`");
        return false;
    }
#line 1215 "src/parse/lex.cc"
yy209:
	++cur;
	cur = yyt1;
#line 308 "../src/parse/lex.re"
	{ name.clear();              return true; }
#line 1221 "src/parse/lex.cc"
yy211:
	yych = (unsigned char)*++cur;
	if (yych == '/') goto yy209;
yy212:
	cur = mar;
	goto yy208;
yy213:
	yych = (unsigned char)*++cur;
	if (yych <= '^') {
		if (yych <= '@') goto yy212;
		if (yych >= '[') goto yy212;
	} else {
		if (yych == '`') goto yy212;
		if (yych >= '{') goto yy212;
	}
yy214:
	++cur;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 128) {
		goto yy214;
	}
	if (yych <= '\r') {
		if (yych <= 0x08) goto yy212;
		if (yych <= '\n') {
			yyt1 = cur;
			goto yy216;
		}
		if (yych <= '\f') goto yy212;
		yyt1 = cur;
	} else {
		if (yych <= ' ') {
			if (yych <= 0x1F) goto yy212;
			yyt1 = cur;
		} else {
			if (yych == '*') {
				yyt1 = cur;
				goto yy218;
			}
			goto yy212;
		}
	}
yy216:
	++cur;
	cur = yyt1;
#line 309 "../src/parse/lex.re"
	{ name.assign(tok + 1, cur); return true; }
#line 1269 "src/parse/lex.cc"
yy218:
	yych = (unsigned char)*++cur;
	if (yych == '/') goto yy216;
	goto yy212;
}
#line 310 "../src/parse/lex.re"

}