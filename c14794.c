int Scanner::scan()
{
    const char *p, *x, *y;
scan:
    tok = cur;
    location = cur_loc();

#line 1706 "src/parse/lex.cc"
{
	unsigned char yych;
	unsigned int yyaccept = 0;
	static const unsigned char yybm[] = {
		  0, 128, 128, 128, 128, 128, 128, 128, 
		128, 144,   0, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		144, 128,   0, 128, 128, 128, 128, 128, 
		128, 128, 128, 128, 128, 128, 128, 128, 
		224, 224, 224, 224, 224, 224, 224, 224, 
		224, 224, 128, 128, 128, 128, 128, 128, 
		128, 160, 160, 160, 160, 160, 160, 160, 
		160, 160, 160, 160, 160, 160, 160, 160, 
		160, 160, 160, 160, 160, 160, 160, 160, 
		160, 160, 160, 128,   0, 128, 128, 160, 
		128, 160, 160, 160, 160, 160, 160, 160, 
		160, 160, 160, 160, 160, 160, 160, 160, 
		160, 160, 160, 160, 160, 160, 160, 160, 
		160, 160, 160, 128, 128, 128, 128, 128, 
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
	if ((lim - cur) < 9) { if (!fill(9)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 16) {
		goto yy278;
	}
	if (yych <= '9') {
		if (yych <= '$') {
			if (yych <= '\r') {
				if (yych <= 0x08) goto yy276;
				if (yych <= '\n') goto yy281;
				if (yych >= '\r') goto yy283;
			} else {
				if (yych <= '!') {
					if (yych >= ' ') goto yy284;
				} else {
					if (yych <= '"') goto yy285;
					if (yych <= '#') goto yy287;
					goto yy288;
				}
			}
		} else {
			if (yych <= '*') {
				if (yych <= '&') {
					if (yych <= '%') goto yy290;
				} else {
					if (yych <= '\'') goto yy291;
					if (yych <= ')') goto yy288;
					goto yy293;
				}
			} else {
				if (yych <= '-') {
					if (yych <= '+') goto yy288;
				} else {
					if (yych <= '.') goto yy294;
					if (yych <= '/') goto yy296;
				}
			}
		}
	} else {
		if (yych <= '[') {
			if (yych <= '=') {
				if (yych <= ':') goto yy297;
				if (yych <= ';') goto yy288;
				if (yych <= '<') goto yy298;
				goto yy300;
			} else {
				if (yych <= '?') {
					if (yych >= '?') goto yy288;
				} else {
					if (yych <= '@') goto yy287;
					if (yych <= 'Z') goto yy301;
					goto yy304;
				}
			}
		} else {
			if (yych <= 'q') {
				if (yych <= '^') {
					if (yych <= '\\') goto yy288;
				} else {
					if (yych != '`') goto yy301;
				}
			} else {
				if (yych <= 'z') {
					if (yych <= 'r') goto yy306;
					goto yy301;
				} else {
					if (yych <= '{') goto yy307;
					if (yych <= '|') goto yy288;
				}
			}
		}
	}
yy276:
	++cur;
yy277:
#line 569 "../src/parse/lex.re"
	{
        msg.error(tok_loc(), "unexpected character: '%c'", *tok);
        exit(1);
    }
#line 1824 "src/parse/lex.cc"
yy278:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 16) {
		goto yy278;
	}
#line 551 "../src/parse/lex.re"
	{ goto scan; }
#line 1834 "src/parse/lex.cc"
yy281:
	yyaccept = 0;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy309;
	} else {
		if (yych <= ' ') goto yy309;
		if (yych == '#') goto yy312;
	}
yy282:
#line 558 "../src/parse/lex.re"
	{
        next_line();
        if (lexer_state == LEX_FLEX_NAME) {
            lexer_state = LEX_NORMAL;
            return TOKEN_FID_END;
        }
        else {
            goto scan;
        }
    }
#line 1856 "src/parse/lex.cc"
yy283:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy281;
	goto yy277;
yy284:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych == 'i') goto yy314;
	if (yych == 'u') goto yy315;
	goto yy277;
yy285:
	++cur;
#line 443 "../src/parse/lex.re"
	{ yylval.regexp = lex_str('"'); return TOKEN_REGEXP; }
#line 1871 "src/parse/lex.cc"
yy287:
	yych = (unsigned char)*++cur;
	if (yych <= '^') {
		if (yych <= '@') goto yy277;
		if (yych <= 'Z') goto yy316;
		goto yy277;
	} else {
		if (yych == '`') goto yy277;
		if (yych <= 'z') goto yy316;
		goto yy277;
	}
yy288:
	++cur;
yy289:
#line 452 "../src/parse/lex.re"
	{ return *tok; }
#line 1888 "src/parse/lex.cc"
yy290:
	yych = (unsigned char)*++cur;
	if (yych == '}') goto yy319;
	goto yy277;
yy291:
	++cur;
#line 442 "../src/parse/lex.re"
	{ yylval.regexp = lex_str('\''); return TOKEN_REGEXP; }
#line 1897 "src/parse/lex.cc"
yy293:
	yych = (unsigned char)*++cur;
	if (yych == '/') goto yy319;
	goto yy289;
yy294:
	++cur;
#line 546 "../src/parse/lex.re"
	{
        yylval.regexp = ast_dot(tok_loc());
        return TOKEN_REGEXP;
    }
#line 1909 "src/parse/lex.cc"
yy296:
	yych = (unsigned char)*++cur;
	if (yych == '*') goto yy321;
	if (yych == '/') goto yy323;
	goto yy289;
yy297:
	yych = (unsigned char)*++cur;
	if (yych == '=') goto yy325;
	goto yy277;
yy298:
	++cur;
#line 435 "../src/parse/lex.re"
	{ return lex_clist(); }
#line 1923 "src/parse/lex.cc"
yy300:
	yyaccept = 2;
	yych = (unsigned char)*(mar = ++cur);
	if (yych == '>') goto yy327;
	goto yy289;
yy301:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
yy302:
	if (yybm[0+yych] & 32) {
		goto yy301;
	}
#line 501 "../src/parse/lex.re"
	{
        if (!globopts->FFlag || lex_namedef_context_re2c()) {
            yylval.str = newstr(tok, cur);
            return TOKEN_ID;
        }
        else if (lex_namedef_context_flex()) {
            yylval.str = newstr(tok, cur);
            lexer_state = LEX_FLEX_NAME;
            return TOKEN_FID;
        }
        else {
            // consume one character, otherwise we risk breaking operator
            // precedence in cases like ab*: it should be a(b)*, not (ab)*
            cur = tok + 1;

            ASTChar c = {static_cast<uint8_t>(tok[0]), tok_loc()};
            std::vector<ASTChar> *str = new std::vector<ASTChar>;
            str->push_back(c);
            yylval.regexp = ast_str(tok_loc(), str, false);
            return TOKEN_REGEXP;
        }
    }
#line 1960 "src/parse/lex.cc"
yy304:
	yych = (unsigned char)*++cur;
	if (yych == '^') goto yy329;
#line 444 "../src/parse/lex.re"
	{ yylval.regexp = lex_cls(false); return TOKEN_REGEXP; }
#line 1966 "src/parse/lex.cc"
yy306:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy331;
	goto yy302;
yy307:
	yyaccept = 3;
	yych = (unsigned char)*(mar = ++cur);
	if (yybm[0+yych] & 64) {
		goto yy334;
	}
	if (yych <= 'Z') {
		if (yych == ',') goto yy332;
		if (yych >= 'A') goto yy336;
	} else {
		if (yych <= '_') {
			if (yych >= '_') goto yy336;
		} else {
			if (yych <= '`') goto yy308;
			if (yych <= 'z') goto yy336;
		}
	}
yy308:
#line 427 "../src/parse/lex.re"
	{ lex_code_in_braces(); return TOKEN_CODE; }
#line 1991 "src/parse/lex.cc"
yy309:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy309;
	} else {
		if (yych <= ' ') goto yy309;
		if (yych == '#') goto yy312;
	}
yy311:
	cur = mar;
	if (yyaccept <= 3) {
		if (yyaccept <= 1) {
			if (yyaccept == 0) {
				goto yy282;
			} else {
				goto yy277;
			}
		} else {
			if (yyaccept == 2) {
				goto yy289;
			} else {
				goto yy308;
			}
		}
	} else {
		if (yyaccept <= 5) {
			if (yyaccept == 4) {
				goto yy326;
			} else {
				goto yy333;
			}
		} else {
			if (yyaccept == 6) {
				goto yy353;
			} else {
				goto yy379;
			}
		}
	}
yy312:
	++cur;
	if ((lim - cur) < 5) { if (!fill(5)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy312;
		goto yy311;
	} else {
		if (yych <= ' ') goto yy312;
		if (yych == 'l') goto yy338;
		goto yy311;
	}
yy314:
	yych = (unsigned char)*++cur;
	if (yych == 'n') goto yy339;
	goto yy311;
yy315:
	yych = (unsigned char)*++cur;
	if (yych == 's') goto yy340;
	goto yy311;
yy316:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 'Z') {
		if (yych <= '/') goto yy318;
		if (yych <= '9') goto yy316;
		if (yych >= 'A') goto yy316;
	} else {
		if (yych <= '_') {
			if (yych >= '_') goto yy316;
		} else {
			if (yych <= '`') goto yy318;
			if (yych <= 'z') goto yy316;
		}
	}
yy318:
#line 447 "../src/parse/lex.re"
	{
        yylval.regexp = ast_tag(tok_loc(), newstr(tok + 1, cur), tok[0] == '#');
        return TOKEN_REGEXP;
    }
#line 2075 "src/parse/lex.cc"
yy319:
	++cur;
#line 440 "../src/parse/lex.re"
	{ tok = cur; return 0; }
#line 2080 "src/parse/lex.cc"
yy321:
	++cur;
#line 438 "../src/parse/lex.re"
	{ lex_c_comment(); goto scan; }
#line 2085 "src/parse/lex.cc"
yy323:
	++cur;
#line 437 "../src/parse/lex.re"
	{ lex_cpp_comment(); goto scan; }
#line 2090 "src/parse/lex.cc"
yy325:
	yyaccept = 4;
	yych = (unsigned char)*(mar = ++cur);
	if (yych == '>') goto yy327;
yy326:
#line 428 "../src/parse/lex.re"
	{ lex_code_indented(); return TOKEN_CODE; }
#line 2098 "src/parse/lex.cc"
yy327:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '@') {
		if (yych <= '\t') {
			if (yych <= 0x08) goto yy311;
			goto yy327;
		} else {
			if (yych == ' ') goto yy327;
			goto yy311;
		}
	} else {
		if (yych <= '_') {
			if (yych <= 'Z') {
				yyt1 = cur;
				goto yy341;
			}
			if (yych <= '^') goto yy311;
			yyt1 = cur;
			goto yy341;
		} else {
			if (yych <= '`') goto yy311;
			if (yych <= 'z') {
				yyt1 = cur;
				goto yy341;
			}
			goto yy311;
		}
	}
yy329:
	++cur;
#line 445 "../src/parse/lex.re"
	{ yylval.regexp = lex_cls(true);  return TOKEN_REGEXP; }
#line 2133 "src/parse/lex.cc"
yy331:
	yych = (unsigned char)*++cur;
	if (yych == '2') goto yy344;
	goto yy302;
yy332:
	++cur;
yy333:
#line 484 "../src/parse/lex.re"
	{
        msg.error(tok_loc(), "illegal closure form, use '{n}', '{n,}', '{n,m}' "
            "where n and m are numbers");
        exit(1);
    }
#line 2147 "src/parse/lex.cc"
yy334:
	++cur;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 64) {
		goto yy334;
	}
	if (yych == ',') {
		yyt1 = cur;
		goto yy345;
	}
	if (yych == '}') goto yy346;
	goto yy311;
yy336:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '^') {
		if (yych <= '9') {
			if (yych <= '/') goto yy311;
			goto yy336;
		} else {
			if (yych <= '@') goto yy311;
			if (yych <= 'Z') goto yy336;
			goto yy311;
		}
	} else {
		if (yych <= 'z') {
			if (yych == '`') goto yy311;
			goto yy336;
		} else {
			if (yych == '}') goto yy348;
			goto yy311;
		}
	}
yy338:
	yych = (unsigned char)*++cur;
	if (yych == 'i') goto yy350;
	goto yy311;
yy339:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy351;
	goto yy311;
yy340:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy352;
	goto yy311;
yy341:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 'Z') {
		if (yych <= '/') goto yy343;
		if (yych <= '9') goto yy341;
		if (yych >= 'A') goto yy341;
	} else {
		if (yych <= '_') {
			if (yych >= '_') goto yy341;
		} else {
			if (yych <= '`') goto yy343;
			if (yych <= 'z') goto yy341;
		}
	}
yy343:
	p = yyt1;
#line 430 "../src/parse/lex.re"
	{
        yylval.str = newstr(p, cur);
        return tok[0] == ':' ? TOKEN_CJUMP : TOKEN_CNEXT;
    }
#line 2218 "src/parse/lex.cc"
yy344:
	yych = (unsigned char)*++cur;
	if (yych == 'c') goto yy354;
	goto yy302;
yy345:
	yyaccept = 5;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '/') goto yy333;
	if (yych <= '9') goto yy355;
	if (yych == '}') goto yy357;
	goto yy333;
yy346:
	++cur;
#line 454 "../src/parse/lex.re"
	{
        if (!s_to_u32_unsafe (tok + 1, cur - 1, yylval.bounds.min)) {
            msg.error(tok_loc(), "repetition count overflow");
            exit(1);
        }
        yylval.bounds.max = yylval.bounds.min;
        return TOKEN_CLOSESIZE;
    }
#line 2241 "src/parse/lex.cc"
yy348:
	++cur;
#line 490 "../src/parse/lex.re"
	{
        if (!globopts->FFlag) {
            msg.error(tok_loc(), "curly braces for names only allowed with -F switch");
            exit(1);
        }
        yylval.str = newstr(tok + 1, cur - 1);
        return TOKEN_ID;
    }
#line 2253 "src/parse/lex.cc"
yy350:
	yych = (unsigned char)*++cur;
	if (yych == 'n') goto yy359;
	goto yy311;
yy351:
	yych = (unsigned char)*++cur;
	if (yych == 'l') goto yy360;
	goto yy311;
yy352:
	yyaccept = 6;
	yych = (unsigned char)*(mar = ++cur);
	if (yych == ':') goto yy361;
yy353:
#line 539 "../src/parse/lex.re"
	{
        msg.error(tok_loc(), "ill-formed use directive: expected `!use`"
            " followed by a colon, a block name, optional spaces, a semicolon,"
            " and finally a space, a newline, or the end of block");
        exit(1);
    }
#line 2274 "src/parse/lex.cc"
yy354:
	yych = (unsigned char)*++cur;
	if (yych == ':') goto yy362;
	goto yy302;
yy355:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '/') goto yy311;
	if (yych <= '9') goto yy355;
	if (yych == '}') goto yy364;
	goto yy311;
yy357:
	++cur;
#line 475 "../src/parse/lex.re"
	{
        if (!s_to_u32_unsafe (tok + 1, cur - 2, yylval.bounds.min)) {
            msg.error(tok_loc(), "repetition lower bound overflow");
            exit(1);
        }
        yylval.bounds.max = std::numeric_limits<uint32_t>::max();
        return TOKEN_CLOSESIZE;
    }
#line 2298 "src/parse/lex.cc"
yy359:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy366;
	goto yy311;
yy360:
	yych = (unsigned char)*++cur;
	if (yych == 'u') goto yy367;
	goto yy311;
yy361:
	yych = (unsigned char)*++cur;
	if (yych <= '^') {
		if (yych <= '@') goto yy311;
		if (yych <= 'Z') {
			yyt1 = cur;
			goto yy368;
		}
		goto yy311;
	} else {
		if (yych == '`') goto yy311;
		if (yych <= 'z') {
			yyt1 = cur;
			goto yy368;
		}
		goto yy311;
	}
yy362:
	++cur;
#line 499 "../src/parse/lex.re"
	{ return TOKEN_CONF; }
#line 2328 "src/parse/lex.cc"
yy364:
	++cur;
	p = yyt1;
#line 463 "../src/parse/lex.re"
	{
        if (!s_to_u32_unsafe (tok + 1, p, yylval.bounds.min)) {
            msg.error(tok_loc(), "repetition lower bound overflow");
            exit(1);
        }
        if (!s_to_u32_unsafe (p + 1, cur - 1, yylval.bounds.max)) {
            msg.error(tok_loc(), "repetition upper bound overflow");
            exit(1);
        }
        return TOKEN_CLOSESIZE;
    }
#line 2344 "src/parse/lex.cc"
yy366:
	yych = (unsigned char)*++cur;
	if (yych <= '0') goto yy371;
	if (yych <= '9') goto yy311;
	goto yy371;
yy367:
	yych = (unsigned char)*++cur;
	if (yych == 'd') goto yy372;
	goto yy311;
yy368:
	++cur;
	if ((lim - cur) < 3) { if (!fill(3)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= ':') {
		if (yych <= 0x1F) {
			if (yych == '\t') {
				yyt2 = cur;
				goto yy373;
			}
			goto yy311;
		} else {
			if (yych <= ' ') {
				yyt2 = cur;
				goto yy373;
			}
			if (yych <= '/') goto yy311;
			if (yych <= '9') goto yy368;
			goto yy311;
		}
	} else {
		if (yych <= '^') {
			if (yych <= ';') {
				yyt2 = cur;
				goto yy375;
			}
			if (yych <= '@') goto yy311;
			if (yych <= 'Z') goto yy368;
			goto yy311;
		} else {
			if (yych == '`') goto yy311;
			if (yych <= 'z') goto yy368;
			goto yy311;
		}
	}
yy370:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
yy371:
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy370;
		goto yy311;
	} else {
		if (yych <= ' ') goto yy370;
		if (yych <= '0') goto yy311;
		if (yych <= '9') {
			yyt1 = cur;
			goto yy376;
		}
		goto yy311;
	}
yy372:
	yych = (unsigned char)*++cur;
	if (yych == 'e') goto yy378;
	goto yy311;
yy373:
	++cur;
	if ((lim - cur) < 3) { if (!fill(3)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy373;
		goto yy311;
	} else {
		if (yych <= ' ') goto yy373;
		if (yych != ';') goto yy311;
	}
yy375:
	yych = (unsigned char)*++cur;
	if (yych <= '\r') {
		if (yych <= 0x08) goto yy311;
		if (yych <= '\n') {
			yyt3 = cur;
			goto yy380;
		}
		if (yych <= '\f') goto yy311;
		yyt3 = cur;
		goto yy380;
	} else {
		if (yych <= ' ') {
			if (yych <= 0x1F) goto yy311;
			yyt3 = cur;
			goto yy380;
		} else {
			if (yych == '*') {
				yyt3 = cur;
				goto yy382;
			}
			goto yy311;
		}
	}
yy376:
	++cur;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '\r') {
		if (yych <= '\t') {
			if (yych <= 0x08) goto yy311;
			goto yy383;
		} else {
			if (yych <= '\n') goto yy385;
			if (yych <= '\f') goto yy311;
			goto yy387;
		}
	} else {
		if (yych <= ' ') {
			if (yych <= 0x1F) goto yy311;
			goto yy383;
		} else {
			if (yych <= '/') goto yy311;
			if (yych <= '9') goto yy376;
			goto yy311;
		}
	}
yy378:
	yyaccept = 7;
	yych = (unsigned char)*(mar = ++cur);
	if (yych == '\t') goto yy388;
	if (yych == ' ') goto yy388;
yy379:
#line 528 "../src/parse/lex.re"
	{
        msg.error(tok_loc(), "ill-formed include directive: expected `!include`"
            " followed by spaces, a double-quoted file path, optional spaces, a"
            " semicolon, and finally a space, a newline, or the end of block");
        exit(1);
    }
#line 2481 "src/parse/lex.cc"
yy380:
	++cur;
	x = yyt1;
	y = yyt2;
	cur = yyt3;
#line 535 "../src/parse/lex.re"
	{
        yylval.str = newstr(x, y); // save the name of the used block
        return TOKEN_BLOCK;
    }
#line 2492 "src/parse/lex.cc"
yy382:
	yych = (unsigned char)*++cur;
	if (yych == '/') goto yy380;
	goto yy311;
yy383:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy383;
		goto yy311;
	} else {
		if (yych <= ' ') goto yy383;
		if (yych == '"') goto yy390;
		goto yy311;
	}
yy385:
	++cur;
	cur = yyt1;
#line 553 "../src/parse/lex.re"
	{
        set_sourceline ();
        return TOKEN_LINE_INFO;
    }
#line 2517 "src/parse/lex.cc"
yy387:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy385;
	goto yy311;
yy388:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy388;
		goto yy311;
	} else {
		if (yych <= ' ') goto yy388;
		if (yych == '"') {
			yyt1 = cur;
			goto yy392;
		}
		goto yy311;
	}
yy390:
	++cur;
	if ((lim - cur) < 3) { if (!fill(3)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yybm[0+yych] & 128) {
		goto yy390;
	}
	if (yych <= '\n') goto yy311;
	if (yych <= '"') goto yy394;
	goto yy395;
yy392:
	++cur;
	if ((lim - cur) < 4) { if (!fill(4)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '!') {
		if (yych <= 0x00) goto yy311;
		if (yych == '\n') goto yy311;
		goto yy392;
	} else {
		if (yych <= '"') goto yy396;
		if (yych == '\\') goto yy397;
		goto yy392;
	}
yy394:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy385;
	if (yych == '\r') goto yy387;
	goto yy311;
yy395:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x00) goto yy311;
	if (yych == '\n') goto yy311;
	goto yy390;
yy396:
	yych = (unsigned char)*++cur;
	if (yych <= 0x1F) {
		if (yych == '\t') {
			yyt2 = cur;
			goto yy398;
		}
		goto yy311;
	} else {
		if (yych <= ' ') {
			yyt2 = cur;
			goto yy398;
		}
		if (yych == ';') {
			yyt2 = cur;
			goto yy400;
		}
		goto yy311;
	}
yy397:
	++cur;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x00) goto yy311;
	if (yych == '\n') goto yy311;
	goto yy392;
yy398:
	++cur;
	if ((lim - cur) < 3) { if (!fill(3)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x1F) {
		if (yych == '\t') goto yy398;
		goto yy311;
	} else {
		if (yych <= ' ') goto yy398;
		if (yych != ';') goto yy311;
	}
yy400:
	yych = (unsigned char)*++cur;
	if (yych <= '\r') {
		if (yych <= 0x08) goto yy311;
		if (yych <= '\n') {
			yyt3 = cur;
			goto yy401;
		}
		if (yych <= '\f') goto yy311;
		yyt3 = cur;
	} else {
		if (yych <= ' ') {
			if (yych <= 0x1F) goto yy311;
			yyt3 = cur;
		} else {
			if (yych == '*') {
				yyt3 = cur;
				goto yy403;
			}
			goto yy311;
		}
	}
yy401:
	++cur;
	x = yyt1;
	y = yyt2;
	cur = yyt3;
#line 524 "../src/parse/lex.re"
	{
        include(getstr(x + 1, y - 1), tok);
        goto scan;
    }
#line 2641 "src/parse/lex.cc"
yy403:
	yych = (unsigned char)*++cur;
	if (yych == '/') goto yy401;
	goto yy311;
}
#line 573 "../src/parse/lex.re"

}