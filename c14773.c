InputBlockKind Scanner::echo(Output &out, std::string &block_name)
{
    const opt_t *opts = out.block().opts;
    code_alc_t &alc = out.allocator;
    const char *x, *y;

    if (is_eof()) return INPUT_END;

next:
    tok = cur;
loop:
    location = cur_loc();
    ptr = cur;

#line 55 "src/parse/lex.cc"
{
	unsigned char yych;
	unsigned int yyaccept = 0;
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
	if ((lim - cur) < 18) { if (!fill(18)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '\r') {
		if (yych <= '\t') {
			if (yych <= 0x00) goto yy2;
			if (yych <= 0x08) goto yy4;
			goto yy6;
		} else {
			if (yych <= '\n') goto yy7;
			if (yych <= '\f') goto yy4;
			goto yy9;
		}
	} else {
		if (yych <= '$') {
			if (yych == ' ') goto yy6;
			goto yy4;
		} else {
			if (yych <= '%') goto yy10;
			if (yych == '/') goto yy11;
			goto yy4;
		}
	}
yy2:
	++cur;
#line 273 "../src/parse/lex.re"
	{
        if (is_eof()) {
            out.wraw(tok, ptr);
            return INPUT_END;
        }
        goto loop;
    }
#line 125 "src/parse/lex.cc"
yy4:
	++cur;
yy5:
#line 293 "../src/parse/lex.re"
	{ goto loop; }
#line 131 "src/parse/lex.cc"
yy6:
	yyaccept = 0;
	yych = (unsigned char)*(mar = ++cur);
	if (yybm[0+yych] & 32) {
		goto yy12;
	}
	if (yych == '%') goto yy15;
	goto yy5;
yy7:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy16;
	} else {
		if (yych <= ' ') goto yy16;
		if (yych == '#') goto yy18;
	}
yy8:
#line 288 "../src/parse/lex.re"
	{
        next_line();
        goto loop;
    }
#line 155 "src/parse/lex.cc"
yy9:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy7;
	goto yy5;
yy10:
	yych = (unsigned char)*++cur;
	if (yych == '{') goto yy20;
	goto yy5;
yy11:
	yyaccept = 0;
	yych = (unsigned char)*(mar = ++cur);
	if (yych == '*') goto yy22;
	goto yy5;
yy12:
	++cur;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 32) {
		goto yy12;
	}
	if (yych == '%') goto yy15;
yy14:
	cur = mar;
	if (yyaccept <= 2) {
		if (yyaccept <= 1) {
			if (yyaccept == 0) {
				goto yy5;
			} else {
				goto yy8;
			}
		} else {
			goto yy173;
		}
	} else {
		if (yyaccept == 3) {
			goto yy175;
		} else {
			goto yy184;
		}
	}
yy15:
	yych = (unsigned char)*++cur;
	if (yych == '{') goto yy20;
	goto yy14;
yy16:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy16;
		goto yy14;
	} else {
		if (yych <= ' ') goto yy16;
		if (yych != '#') goto yy14;
	}
yy18:
	++cur;
	if ((lim - cur) < 5) { if (!fill(5)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy18;
		goto yy14;
	} else {
		if (yych <= ' ') goto yy18;
		if (yych == 'l') goto yy23;
		goto yy14;
	}
yy20:
	++cur;
#line 139 "../src/parse/lex.re"
	{
        if (pos != ptr) {
            // re2c does not parse user-defined code outside of re2c blocks, therefore it
            // can confuse `%{` in the middle of a string or a comment with a block start.
            // To avoid this recognize `%{` as a block start only on a new line, possibly
            // preceded by whitespaces.
            goto loop;
        }
        out.wraw(tok, ptr);
        block_name.clear();
        return INPUT_GLOBAL;
    }
#line 238 "src/parse/lex.cc"
yy22:
	yych = (unsigned char)*++cur;
	if (yych == '!') goto yy24;
	goto yy14;
yy23:
	yych = (unsigned char)*++cur;
	if (yych == 'i') goto yy25;
	goto yy14;
yy24:
	yych = (unsigned char)*++cur;
	switch (yych) {
	case 'c':	goto yy26;
	case 'g':	goto yy27;
	case 'h':	goto yy28;
	case 'i':	goto yy29;
	case 'l':	goto yy30;
	case 'm':	goto yy31;
	case 'r':	goto yy32;
	case 's':	goto yy33;
	case 't':	goto yy34;
	case 'u':	goto yy35;
	default:	goto yy14;
	}
yy25:
	yych = (unsigned char)*++cur;
	if (yych == 'n') goto yy36;
	goto yy14;
yy26:
	yych = (unsigned char)*++cur;
	if (yych == 'o') goto yy37;
	goto yy14;
yy27:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy38;
	goto yy14;
yy28:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy39;
	goto yy14;
yy29:
	yych = (unsigned char)*++cur;
	if (yych == 'g') goto yy40;
	if (yych == 'n') goto yy41;
	goto yy14;
yy30:
	yych = (unsigned char)*++cur;
	if (yych == 'o') goto yy42;
	goto yy14;
yy31:
	yych = (unsigned char)*++cur;
	if (yych == 'a') goto yy43;
	if (yych == 't') goto yy44;
	goto yy14;
yy32:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy45;
	if (yych == 'u') goto yy46;
	goto yy14;
yy33:
	yych = (unsigned char)*++cur;
	if (yych == 't') goto yy47;
	goto yy14;
yy34:
	yych = (unsigned char)*++cur;
	if (yych == 'y') goto yy48;
	goto yy14;
yy35:
	yych = (unsigned char)*++cur;
	if (yych == 's') goto yy49;
	goto yy14;
yy36:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy50;
	goto yy14;
yy37:
	yych = (unsigned char)*++cur;
	if (yych == 'n') goto yy51;
	goto yy14;
yy38:
	yych = (unsigned char)*++cur;
	if (yych == 't') goto yy52;
	goto yy14;
yy39:
	yych = (unsigned char)*++cur;
	if (yych == 'a') goto yy53;
	goto yy14;
yy40:
	yych = (unsigned char)*++cur;
	if (yych == 'n') goto yy54;
	goto yy14;
yy41:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy55;
	goto yy14;
yy42:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy56;
	goto yy14;
yy43:
	yych = (unsigned char)*++cur;
	if (yych == 'x') goto yy57;
	goto yy14;
yy44:
	yych = (unsigned char)*++cur;
	if (yych == 'a') goto yy58;
	goto yy14;
yy45:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy59;
	goto yy14;
yy46:
	yych = (unsigned char)*++cur;
	if (yych == 'l') goto yy60;
	goto yy14;
yy47:
	yych = (unsigned char)*++cur;
	if (yych == 'a') goto yy61;
	goto yy14;
yy48:
	yych = (unsigned char)*++cur;
	if (yych == 'p') goto yy62;
	goto yy14;
yy49:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy63;
	goto yy14;
yy50:
	yych = (unsigned char)*++cur;
	if (yych <= '0') goto yy65;
	if (yych <= '9') goto yy14;
	goto yy65;
yy51:
	yych = (unsigned char)*++cur;
	if (yych == 'd') goto yy66;
	goto yy14;
yy52:
	yych = (unsigned char)*++cur;
	if (yych == 's') goto yy67;
	goto yy14;
yy53:
	yych = (unsigned char)*++cur;
	if (yych == 'd') goto yy68;
	goto yy14;
yy54:
	yych = (unsigned char)*++cur;
	if (yych == 'o') goto yy69;
	goto yy14;
yy55:
	yych = (unsigned char)*++cur;
	if (yych == 'l') goto yy70;
	goto yy14;
yy56:
	yych = (unsigned char)*++cur;
	if (yych == 'a') goto yy71;
	goto yy14;
yy57:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy72;
	if (yych == 'n') goto yy73;
	goto yy14;
yy58:
	yych = (unsigned char)*++cur;
	if (yych == 'g') goto yy74;
	goto yy14;
yy59:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy75;
	goto yy14;
yy60:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy77;
	goto yy14;
yy61:
	yych = (unsigned char)*++cur;
	if (yych == 'g') goto yy78;
	goto yy14;
yy62:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy79;
	goto yy14;
yy63:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy80;
	goto yy14;
yy64:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
yy65:
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy64;
		goto yy14;
	} else {
		if (yych <= ' ') goto yy64;
		if (yych <= '0') goto yy14;
		if (yych <= '9') {
			yyt1 = cur;
			goto yy81;
		}
		goto yy14;
	}
yy66:
	yych = (unsigned char)*++cur;
	if (yych == 'i') goto yy83;
	goto yy14;
yy67:
	yych = (unsigned char)*++cur;
	if (yych == 't') goto yy84;
	goto yy14;
yy68:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy85;
	goto yy14;
yy69:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy86;
	goto yy14;
yy70:
	yych = (unsigned char)*++cur;
	if (yych == 'u') goto yy87;
	goto yy14;
yy71:
	yych = (unsigned char)*++cur;
	if (yych == 'l') goto yy88;
	goto yy14;
yy72:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy89;
	goto yy14;
yy73:
	yych = (unsigned char)*++cur;
	if (yych == 'm') goto yy90;
	goto yy14;
yy74:
	yych = (unsigned char)*++cur;
	if (yych == 's') goto yy91;
	goto yy14;
yy75:
	++cur;
#line 152 "../src/parse/lex.re"
	{
        out.wraw(tok, ptr);
        if (!lex_opt_name(block_name)) return INPUT_ERROR;
        if (block_name == "local") {
            msg.error(cur_loc(), "ill-formed local block, expected `local:re2c`");
            return INPUT_ERROR;
        }
        return INPUT_GLOBAL;
    }
#line 488 "src/parse/lex.cc"
yy77:
	yych = (unsigned char)*++cur;
	if (yych == 's') goto yy92;
	goto yy14;
yy78:
	yych = (unsigned char)*++cur;
	if (yych == 's') goto yy93;
	goto yy14;
yy79:
	yych = (unsigned char)*++cur;
	if (yych == 's') goto yy94;
	goto yy14;
yy80:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy95;
	goto yy14;
yy81:
	++cur;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 64) {
		goto yy81;
	}
	if (yych <= '\f') {
		if (yych <= 0x08) goto yy14;
		if (yych <= '\t') goto yy96;
		if (yych <= '\n') goto yy98;
		goto yy14;
	} else {
		if (yych <= '\r') goto yy100;
		if (yych == ' ') goto yy96;
		goto yy14;
	}
yy83:
	yych = (unsigned char)*++cur;
	if (yych == 't') goto yy101;
	goto yy14;
yy84:
	yych = (unsigned char)*++cur;
	if (yych == 'a') goto yy102;
	goto yy14;
yy85:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy103;
	goto yy14;
yy86:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy104;
	goto yy14;
yy87:
	yych = (unsigned char)*++cur;
	if (yych == 'd') goto yy105;
	goto yy14;
yy88:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy106;
	goto yy14;
yy89:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy107;
	goto yy14;
yy90:
	yych = (unsigned char)*++cur;
	if (yych == 'a') goto yy108;
	goto yy14;
yy91:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy109;
	goto yy14;
yy92:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy110;
	goto yy14;
yy93:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy111;
	goto yy14;
yy94:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy112;
	goto yy14;
yy95:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy113;
	goto yy14;
yy96:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy96;
		goto yy14;
	} else {
		if (yych <= ' ') goto yy96;
		if (yych == '"') goto yy114;
		goto yy14;
	}
yy98:
	++cur;
	cur = yyt1;
#line 281 "../src/parse/lex.re"
	{
        out.wraw(tok, ptr);
        out.wdelay_stmt(0, code_newline(alc));
        set_sourceline();
        goto next;
    }
#line 596 "src/parse/lex.cc"
yy100:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy98;
	goto yy14;
yy101:
	yych = (unsigned char)*++cur;
	if (yych == 'i') goto yy116;
	goto yy14;
yy102:
	yych = (unsigned char)*++cur;
	if (yych == 't') goto yy117;
	goto yy14;
yy103:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy118;
	goto yy14;
yy104:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy119;
	goto yy14;
yy105:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy120;
	goto yy14;
yy106:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy121;
	goto yy14;
yy107:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy122;
	goto yy14;
yy108:
	yych = (unsigned char)*++cur;
	if (yych == 't') goto yy123;
	goto yy14;
yy109:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy124;
	goto yy14;
yy110:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy125;
	goto yy14;
yy111:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy126;
	goto yy14;
yy112:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy127;
	goto yy14;
yy113:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy128;
	goto yy14;
yy114:
	++cur;
	if ((lim - cur) < 3) { if (!fill(3)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 128) {
		goto yy114;
	}
	if (yych <= '\n') goto yy14;
	if (yych <= '"') goto yy129;
	goto yy130;
yy116:
	yych = (unsigned char)*++cur;
	if (yych == 'o') goto yy131;
	goto yy14;
yy117:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy132;
	goto yy14;
yy118:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy133;
	goto yy14;
yy119:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy134;
	goto yy14;
yy120:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy135;
	goto yy14;
yy121:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy136;
	goto yy14;
yy122:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy137;
	goto yy14;
yy123:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy139;
	goto yy14;
yy124:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy140;
	goto yy14;
yy125:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy141;
	goto yy14;
yy126:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy142;
	goto yy14;
yy127:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy143;
	goto yy14;
yy128:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy144;
	goto yy14;
yy129:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy98;
	if (yych == '\r') goto yy100;
	goto yy14;
yy130:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x00) goto yy14;
	if (yych == '\n') goto yy14;
	goto yy114;
yy131:
	yych = (unsigned char)*++cur;
	if (yych == 'n') goto yy79;
	goto yy14;
yy132:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy146;
	goto yy14;
yy133:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy147;
	goto yy14;
yy134:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy148;
	goto yy14;
yy135:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy149;
	goto yy14;
yy136:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy150;
	goto yy14;
yy137:
	++cur;
#line 180 "../src/parse/lex.re"
	{
        if (!lex_block(out, CODE_MAXFILL, 0, DCONF_FORMAT)) return INPUT_ERROR;
        goto next;
    }
#line 758 "src/parse/lex.cc"
yy139:
	yych = (unsigned char)*++cur;
	if (yych == 'h') goto yy151;
	goto yy14;
yy140:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy152;
	goto yy14;
yy141:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy153;
	goto yy14;
yy142:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy154;
	goto yy14;
yy143:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy155;
	goto yy14;
yy144:
	++cur;
#line 174 "../src/parse/lex.re"
	{
        out.wraw(tok, ptr);
        if (!lex_opt_name(block_name)) return INPUT_ERROR;
        return INPUT_USE;
    }
#line 787 "src/parse/lex.cc"
yy146:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy156;
	goto yy14;
yy147:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy157;
	goto yy14;
yy148:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy158;
	goto yy14;
yy149:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy159;
	goto yy14;
yy150:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy160;
	goto yy14;
yy151:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy162;
	goto yy14;
yy152:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy163;
	goto yy14;
yy153:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy165;
	goto yy14;
yy154:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy167;
	goto yy14;
yy155:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy169;
	goto yy14;
yy156:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy171;
	goto yy14;
yy157:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy172;
	goto yy14;
yy158:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy174;
	goto yy14;
yy159:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy176;
	goto yy14;
yy160:
	++cur;
#line 162 "../src/parse/lex.re"
	{
        out.wraw(tok, ptr);
        if (!lex_opt_name(block_name)) return INPUT_ERROR;
        return INPUT_LOCAL;
    }
#line 852 "src/parse/lex.cc"
yy162:
	yych = (unsigned char)*++cur;
	if (yych == 'r') goto yy177;
	goto yy14;
yy163:
	++cur;
#line 196 "../src/parse/lex.re"
	{
        uint32_t allow = DCONF_FORMAT | DCONF_SEPARATOR;
        if (!lex_block(out, CODE_MTAGS, 0, allow)) return INPUT_ERROR;
        goto next;
    }
#line 865 "src/parse/lex.cc"
yy165:
	++cur;
#line 168 "../src/parse/lex.re"
	{
        out.wraw(tok, ptr);
        if (!lex_opt_name(block_name)) return INPUT_ERROR;
        return INPUT_RULES;
    }
#line 874 "src/parse/lex.cc"
yy167:
	++cur;
#line 190 "../src/parse/lex.re"
	{
        uint32_t allow = DCONF_FORMAT | DCONF_SEPARATOR;
        if (!lex_block(out, CODE_STAGS, 0, allow)) return INPUT_ERROR;
        goto next;
    }
#line 883 "src/parse/lex.cc"
yy169:
	++cur;
#line 202 "../src/parse/lex.re"
	{
        out.cond_enum_autogen = false;
        out.warn_condition_order = false; // see note [condition order]
        uint32_t allow = DCONF_FORMAT | DCONF_SEPARATOR;
        if (!lex_block(out, CODE_COND_ENUM, opts->topIndent, allow)) return INPUT_ERROR;
        goto next;
    }
#line 894 "src/parse/lex.cc"
yy171:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy178;
	goto yy14;
yy172:
	yyaccept = 2;
	yych = (unsigned char)*(mar = ++cur);
	if (yych == ':') goto yy179;
yy173:
#line 241 "../src/parse/lex.re"
	{
        msg.error(cur_loc(), "ill-formed header directive: expected"
            " `/*!header:re2c:<on|off>` followed by a space, a newline or the"
            " end of block `*" "/`");
        return INPUT_ERROR;
    }
#line 911 "src/parse/lex.cc"
yy174:
	yyaccept = 3;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '\r') {
		if (yych <= 0x08) goto yy175;
		if (yych <= '\n') {
			yyt1 = cur;
			goto yy180;
		}
		if (yych >= '\r') {
			yyt1 = cur;
			goto yy180;
		}
	} else {
		if (yych <= ' ') {
			if (yych >= ' ') {
				yyt1 = cur;
				goto yy180;
			}
		} else {
			if (yych == '*') {
				yyt1 = cur;
				goto yy182;
			}
		}
	}
yy175:
#line 267 "../src/parse/lex.re"
	{
        msg.error(cur_loc(), "ill-formed start of `ignore:re2c` block: expected"
            " a space, a newline, or the end of block `*" "/`");
        return INPUT_ERROR;
    }
#line 945 "src/parse/lex.cc"
yy176:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy183;
	goto yy14;
yy177:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy185;
	goto yy14;
yy178:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy186;
	goto yy14;
yy179:
	yych = (unsigned char)*++cur;
	if (yych == 'o') goto yy188;
	goto yy14;
yy180:
	++cur;
	cur = yyt1;
#line 261 "../src/parse/lex.re"
	{
        out.wraw(tok, ptr);
        // allows arbitrary garbage before the end of the comment
        if (!lex_block_end(out, true)) return INPUT_ERROR;
        goto next;
    }
#line 972 "src/parse/lex.cc"
yy182:
	yych = (unsigned char)*++cur;
	if (yych == '/') goto yy180;
	goto yy14;
yy183:
	yyaccept = 4;
	yych = (unsigned char)*(mar = ++cur);
	if (yych == '\t') goto yy189;
	if (yych == ' ') goto yy189;
yy184:
#line 255 "../src/parse/lex.re"
	{
        msg.error(cur_loc(), "ill-formed include directive: expected"
            " `/*!include:re2c \"<file>\" *" "/`");
        return INPUT_ERROR;
    }
#line 989 "src/parse/lex.cc"
yy185:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy191;
	goto yy14;
yy186:
	++cur;
#line 210 "../src/parse/lex.re"
	{
        out.state_goto = true;
        if (!opts->fFlag) {
            msg.error(cur_loc(), "`getstate:re2c` without `-f --storable-state` option");
            return INPUT_ERROR;
        }
        if (opts->loop_switch) {
            msg.error(cur_loc(), "`getstate:re2c` is incompatible with the --loop-switch "
                "option, as it requires cross-block transitions that are unsupported "
                "without the `goto` statement");
            return INPUT_ERROR;
        }
        if (!lex_block(out, CODE_STATE_GOTO, opts->topIndent, 0)) return INPUT_ERROR;
        goto next;
    }
#line 1012 "src/parse/lex.cc"
yy188:
	yych = (unsigned char)*++cur;
	if (yych == 'f') goto yy192;
	if (yych == 'n') goto yy193;
	goto yy14;
yy189:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy189;
		goto yy14;
	} else {
		if (yych <= ' ') goto yy189;
		if (yych == '"') {
			yyt1 = cur;
			goto yy195;
		}
		goto yy14;
	}
yy191:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy197;
	goto yy14;
yy192:
	yych = (unsigned char)*++cur;
	if (yych == 'f') goto yy199;
	goto yy14;
yy193:
	++cur;
#line 226 "../src/parse/lex.re"
	{
        out.wraw(tok, ptr);
        out.header_mode(true);
        out.need_header = true;
        if (!lex_block_end(out)) return INPUT_ERROR;
        goto next;
    }
#line 1051 "src/parse/lex.cc"
yy195:
	++cur;
	if ((lim - cur) < 3) { if (!fill(3)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '!') {
		if (yych <= 0x00) goto yy14;
		if (yych == '\n') goto yy14;
		goto yy195;
	} else {
		if (yych <= '"') goto yy201;
		if (yych == '\\') goto yy202;
		goto yy195;
	}
yy197:
	++cur;
#line 185 "../src/parse/lex.re"
	{
        if (!lex_block(out, CODE_MAXNMATCH, 0, DCONF_FORMAT)) return INPUT_ERROR;
        goto next;
    }
#line 1072 "src/parse/lex.cc"
yy199:
	++cur;
#line 234 "../src/parse/lex.re"
	{
        out.wraw(tok, ptr);
        out.header_mode(false);
        out.wdelay_stmt(0, code_line_info_input(alc, cur_loc()));
        if (!lex_block_end(out)) return INPUT_ERROR;
        goto next;
    }
#line 1083 "src/parse/lex.cc"
yy201:
	yych = (unsigned char)*++cur;
	if (yych <= '\r') {
		if (yych <= 0x08) goto yy14;
		if (yych <= '\n') {
			yyt2 = cur;
			goto yy203;
		}
		if (yych <= '\f') goto yy14;
		yyt2 = cur;
		goto yy203;
	} else {
		if (yych <= ' ') {
			if (yych <= 0x1F) goto yy14;
			yyt2 = cur;
			goto yy203;
		} else {
			if (yych == '*') {
				yyt2 = cur;
				goto yy205;
			}
			goto yy14;
		}
	}
yy202:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x00) goto yy14;
	if (yych == '\n') goto yy14;
	goto yy195;
yy203:
	++cur;
	x = yyt1;
	cur = yyt2;
	y = yyt2;
#line 248 "../src/parse/lex.re"
	{
        out.wraw(tok, ptr);
        if (!lex_block_end(out)) return INPUT_ERROR;
        include(getstr(x + 1, y - 1), ptr);
        out.wdelay_stmt(0, code_line_info_input(alc, cur_loc()));
        goto next;
    }
#line 1128 "src/parse/lex.cc"
yy205:
	yych = (unsigned char)*++cur;
	if (yych == '/') goto yy203;
	goto yy14;
}
#line 294 "../src/parse/lex.re"

}