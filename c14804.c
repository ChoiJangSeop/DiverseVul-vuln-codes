bool Scanner::lex_block_end(Output &out, bool allow_garbage)
{
    bool multiline = false;
loop:

#line 1419 "src/parse/lex.cc"
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
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 128) {
		goto yy234;
	}
	if (yych <= '\f') {
		if (yych <= 0x08) goto yy232;
		if (yych <= '\n') goto yy237;
	} else {
		if (yych <= '\r') goto yy239;
		if (yych == '*') goto yy240;
	}
yy232:
	++cur;
yy233:
#line 356 "../src/parse/lex.re"
	{
        if (allow_garbage && !is_eof()) goto loop;
        msg.error(cur_loc(), "ill-formed end of block: expected optional"
            " whitespaces followed by `*" "/`");
        return false;
    }
#line 1478 "src/parse/lex.cc"
yy234:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 128) {
		goto yy234;
	}
#line 366 "../src/parse/lex.re"
	{ goto loop; }
#line 1488 "src/parse/lex.cc"
yy237:
	++cur;
#line 367 "../src/parse/lex.re"
	{ next_line(); multiline = true; goto loop; }
#line 1493 "src/parse/lex.cc"
yy239:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy237;
	goto yy233;
yy240:
	yych = (unsigned char)*++cur;
	if (yych != '/') goto yy233;
	++cur;
#line 362 "../src/parse/lex.re"
	{
        if (multiline) out.wdelay_stmt(0, code_line_info_input(out.allocator, cur_loc()));
        return true;
    }
#line 1507 "src/parse/lex.cc"
}
#line 368 "../src/parse/lex.re"

}