bool Scanner::lex_str_chr(char quote, ASTChar &ast)
{
    tok = cur;
    ast.loc = cur_loc();
    #line 837 "../src/parse/lex.re"

    if (globopts->input_encoding == Enc::ASCII) {
        
#line 4645 "src/parse/lex.cc"
{
	unsigned char yych;
	unsigned int yyaccept = 0;
	if ((lim - cur) < 10) { if (!fill(10)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '\f') {
		if (yych <= 0x00) goto yy711;
		if (yych == '\n') goto yy715;
		goto yy713;
	} else {
		if (yych <= '\r') goto yy717;
		if (yych == '\\') goto yy718;
		goto yy713;
	}
yy711:
	++cur;
#line 819 "../src/parse/lex.re"
	{ fail_if_eof(); ast.chr = 0; return true; }
#line 4664 "src/parse/lex.cc"
yy713:
	++cur;
yy714:
#line 821 "../src/parse/lex.re"
	{ ast.chr = decode(tok); return tok[0] != quote; }
#line 4670 "src/parse/lex.cc"
yy715:
	++cur;
#line 813 "../src/parse/lex.re"
	{ msg.error(ast.loc, "newline in character string"); exit(1); }
#line 4675 "src/parse/lex.cc"
yy717:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy715;
	goto yy714;
yy718:
	yych = (unsigned char)*++cur;
	if (yych <= '`') {
		if (yych <= '3') {
			if (yych <= '\n') {
				if (yych <= 0x00) goto yy719;
				if (yych <= '\t') goto yy720;
				goto yy715;
			} else {
				if (yych == '\r') goto yy722;
				if (yych <= '/') goto yy720;
				goto yy723;
			}
		} else {
			if (yych <= 'W') {
				if (yych <= '7') goto yy725;
				if (yych == 'U') goto yy726;
				goto yy720;
			} else {
				if (yych <= 'X') goto yy728;
				if (yych == '\\') goto yy729;
				goto yy720;
			}
		}
	} else {
		if (yych <= 'q') {
			if (yych <= 'e') {
				if (yych <= 'a') goto yy731;
				if (yych <= 'b') goto yy733;
				goto yy720;
			} else {
				if (yych <= 'f') goto yy735;
				if (yych == 'n') goto yy737;
				goto yy720;
			}
		} else {
			if (yych <= 'u') {
				if (yych <= 'r') goto yy739;
				if (yych <= 's') goto yy720;
				if (yych <= 't') goto yy741;
				goto yy728;
			} else {
				if (yych <= 'v') goto yy743;
				if (yych == 'x') goto yy745;
				goto yy720;
			}
		}
	}
yy719:
#line 816 "../src/parse/lex.re"
	{ msg.error(ast.loc, "syntax error in escape sequence"); exit(1); }
#line 4731 "src/parse/lex.cc"
yy720:
	++cur;
yy721:
#line 832 "../src/parse/lex.re"
	{
        ast.chr = decode(tok + 1);
        if (tok[1] != quote) msg.warn.useless_escape(ast.loc, tok, cur);
        return true;
    }
#line 4741 "src/parse/lex.cc"
yy722:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy715;
	goto yy721;
yy723:
	yyaccept = 0;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '/') goto yy724;
	if (yych <= '7') goto yy746;
yy724:
#line 815 "../src/parse/lex.re"
	{ msg.error(ast.loc, "syntax error in octal escape sequence"); exit(1); }
#line 4754 "src/parse/lex.cc"
yy725:
	++cur;
	goto yy724;
yy726:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy727;
		if (yych <= '9') goto yy748;
	} else {
		if (yych <= 'F') goto yy748;
		if (yych <= '`') goto yy727;
		if (yych <= 'f') goto yy748;
	}
yy727:
#line 814 "../src/parse/lex.re"
	{ msg.error(ast.loc, "syntax error in hexadecimal escape sequence"); exit(1); }
#line 4772 "src/parse/lex.cc"
yy728:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy727;
		if (yych <= '9') goto yy749;
		goto yy727;
	} else {
		if (yych <= 'F') goto yy749;
		if (yych <= '`') goto yy727;
		if (yych <= 'f') goto yy749;
		goto yy727;
	}
yy729:
	++cur;
#line 831 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\\'); return true; }
#line 4790 "src/parse/lex.cc"
yy731:
	++cur;
#line 824 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\a'); return true; }
#line 4795 "src/parse/lex.cc"
yy733:
	++cur;
#line 825 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\b'); return true; }
#line 4800 "src/parse/lex.cc"
yy735:
	++cur;
#line 826 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\f'); return true; }
#line 4805 "src/parse/lex.cc"
yy737:
	++cur;
#line 827 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\n'); return true; }
#line 4810 "src/parse/lex.cc"
yy739:
	++cur;
#line 828 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\r'); return true; }
#line 4815 "src/parse/lex.cc"
yy741:
	++cur;
#line 829 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\t'); return true; }
#line 4820 "src/parse/lex.cc"
yy743:
	++cur;
#line 830 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\v'); return true; }
#line 4825 "src/parse/lex.cc"
yy745:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy727;
		if (yych <= '9') goto yy750;
		goto yy727;
	} else {
		if (yych <= 'F') goto yy750;
		if (yych <= '`') goto yy727;
		if (yych <= 'f') goto yy750;
		goto yy727;
	}
yy746:
	yych = (unsigned char)*++cur;
	if (yych <= '/') goto yy747;
	if (yych <= '7') goto yy751;
yy747:
	cur = mar;
	if (yyaccept == 0) {
		goto yy724;
	} else {
		goto yy727;
	}
yy748:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy747;
		if (yych <= '9') goto yy753;
		goto yy747;
	} else {
		if (yych <= 'F') goto yy753;
		if (yych <= '`') goto yy747;
		if (yych <= 'f') goto yy753;
		goto yy747;
	}
yy749:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy747;
		if (yych <= '9') goto yy754;
		goto yy747;
	} else {
		if (yych <= 'F') goto yy754;
		if (yych <= '`') goto yy747;
		if (yych <= 'f') goto yy754;
		goto yy747;
	}
yy750:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy747;
		if (yych <= '9') goto yy755;
		goto yy747;
	} else {
		if (yych <= 'F') goto yy755;
		if (yych <= '`') goto yy747;
		if (yych <= 'f') goto yy755;
		goto yy747;
	}
yy751:
	++cur;
#line 823 "../src/parse/lex.re"
	{ ast.chr = unesc_oct(tok, cur); return true; }
#line 4890 "src/parse/lex.cc"
yy753:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy747;
		if (yych <= '9') goto yy757;
		goto yy747;
	} else {
		if (yych <= 'F') goto yy757;
		if (yych <= '`') goto yy747;
		if (yych <= 'f') goto yy757;
		goto yy747;
	}
yy754:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy747;
		if (yych <= '9') goto yy750;
		goto yy747;
	} else {
		if (yych <= 'F') goto yy750;
		if (yych <= '`') goto yy747;
		if (yych <= 'f') goto yy750;
		goto yy747;
	}
yy755:
	++cur;
#line 822 "../src/parse/lex.re"
	{ ast.chr = unesc_hex(tok, cur); return true; }
#line 4919 "src/parse/lex.cc"
yy757:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy747;
		if (yych >= ':') goto yy747;
	} else {
		if (yych <= 'F') goto yy758;
		if (yych <= '`') goto yy747;
		if (yych >= 'g') goto yy747;
	}
yy758:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy747;
		if (yych <= '9') goto yy749;
		goto yy747;
	} else {
		if (yych <= 'F') goto yy749;
		if (yych <= '`') goto yy747;
		if (yych <= 'f') goto yy749;
		goto yy747;
	}
}
#line 839 "../src/parse/lex.re"

    }
    else {
        
#line 4948 "src/parse/lex.cc"
{
	unsigned char yych;
	unsigned int yyaccept = 0;
	if ((lim - cur) < 10) { if (!fill(10)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x7F) {
		if (yych <= '\f') {
			if (yych <= 0x00) goto yy761;
			if (yych == '\n') goto yy765;
			goto yy763;
		} else {
			if (yych <= '\r') goto yy767;
			if (yych == '\\') goto yy768;
			goto yy763;
		}
	} else {
		if (yych <= 0xEF) {
			if (yych <= 0xC1) goto yy770;
			if (yych <= 0xDF) goto yy772;
			if (yych <= 0xE0) goto yy773;
			goto yy774;
		} else {
			if (yych <= 0xF0) goto yy775;
			if (yych <= 0xF3) goto yy776;
			if (yych <= 0xF4) goto yy777;
			goto yy770;
		}
	}
yy761:
	++cur;
#line 819 "../src/parse/lex.re"
	{ fail_if_eof(); ast.chr = 0; return true; }
#line 4981 "src/parse/lex.cc"
yy763:
	++cur;
yy764:
#line 821 "../src/parse/lex.re"
	{ ast.chr = decode(tok); return tok[0] != quote; }
#line 4987 "src/parse/lex.cc"
yy765:
	++cur;
#line 813 "../src/parse/lex.re"
	{ msg.error(ast.loc, "newline in character string"); exit(1); }
#line 4992 "src/parse/lex.cc"
yy767:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy765;
	goto yy764;
yy768:
	yyaccept = 0;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 'f') {
		if (yych <= 'T') {
			if (yych <= '\f') {
				if (yych <= 0x00) goto yy769;
				if (yych == '\n') goto yy765;
				goto yy778;
			} else {
				if (yych <= '/') {
					if (yych <= '\r') goto yy780;
					goto yy778;
				} else {
					if (yych <= '3') goto yy781;
					if (yych <= '7') goto yy783;
					goto yy778;
				}
			}
		} else {
			if (yych <= '\\') {
				if (yych <= 'W') {
					if (yych <= 'U') goto yy784;
					goto yy778;
				} else {
					if (yych <= 'X') goto yy786;
					if (yych <= '[') goto yy778;
					goto yy787;
				}
			} else {
				if (yych <= 'a') {
					if (yych <= '`') goto yy778;
					goto yy789;
				} else {
					if (yych <= 'b') goto yy791;
					if (yych <= 'e') goto yy778;
					goto yy793;
				}
			}
		}
	} else {
		if (yych <= 'w') {
			if (yych <= 'r') {
				if (yych == 'n') goto yy795;
				if (yych <= 'q') goto yy778;
				goto yy797;
			} else {
				if (yych <= 't') {
					if (yych <= 's') goto yy778;
					goto yy799;
				} else {
					if (yych <= 'u') goto yy786;
					if (yych <= 'v') goto yy801;
					goto yy778;
				}
			}
		} else {
			if (yych <= 0xE0) {
				if (yych <= 0x7F) {
					if (yych <= 'x') goto yy803;
					goto yy778;
				} else {
					if (yych <= 0xC1) goto yy769;
					if (yych <= 0xDF) goto yy804;
					goto yy806;
				}
			} else {
				if (yych <= 0xF0) {
					if (yych <= 0xEF) goto yy807;
					goto yy808;
				} else {
					if (yych <= 0xF3) goto yy809;
					if (yych <= 0xF4) goto yy810;
				}
			}
		}
	}
yy769:
#line 816 "../src/parse/lex.re"
	{ msg.error(ast.loc, "syntax error in escape sequence"); exit(1); }
#line 5077 "src/parse/lex.cc"
yy770:
	++cur;
yy771:
#line 817 "../src/parse/lex.re"
	{ msg.error(ast.loc, "syntax error"); exit(1); }
#line 5083 "src/parse/lex.cc"
yy772:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy771;
	if (yych <= 0xBF) goto yy763;
	goto yy771;
yy773:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x9F) goto yy771;
	if (yych <= 0xBF) goto yy811;
	goto yy771;
yy774:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x7F) goto yy771;
	if (yych <= 0xBF) goto yy811;
	goto yy771;
yy775:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x8F) goto yy771;
	if (yych <= 0xBF) goto yy812;
	goto yy771;
yy776:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x7F) goto yy771;
	if (yych <= 0xBF) goto yy812;
	goto yy771;
yy777:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x7F) goto yy771;
	if (yych <= 0x8F) goto yy812;
	goto yy771;
yy778:
	++cur;
yy779:
#line 832 "../src/parse/lex.re"
	{
        ast.chr = decode(tok + 1);
        if (tok[1] != quote) msg.warn.useless_escape(ast.loc, tok, cur);
        return true;
    }
#line 5128 "src/parse/lex.cc"
yy780:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy765;
	goto yy779;
yy781:
	yyaccept = 2;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '/') goto yy782;
	if (yych <= '7') goto yy813;
yy782:
#line 815 "../src/parse/lex.re"
	{ msg.error(ast.loc, "syntax error in octal escape sequence"); exit(1); }
#line 5141 "src/parse/lex.cc"
yy783:
	++cur;
	goto yy782;
yy784:
	yyaccept = 3;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy785;
		if (yych <= '9') goto yy814;
	} else {
		if (yych <= 'F') goto yy814;
		if (yych <= '`') goto yy785;
		if (yych <= 'f') goto yy814;
	}
yy785:
#line 814 "../src/parse/lex.re"
	{ msg.error(ast.loc, "syntax error in hexadecimal escape sequence"); exit(1); }
#line 5159 "src/parse/lex.cc"
yy786:
	yyaccept = 3;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy785;
		if (yych <= '9') goto yy815;
		goto yy785;
	} else {
		if (yych <= 'F') goto yy815;
		if (yych <= '`') goto yy785;
		if (yych <= 'f') goto yy815;
		goto yy785;
	}
yy787:
	++cur;
#line 831 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\\'); return true; }
#line 5177 "src/parse/lex.cc"
yy789:
	++cur;
#line 824 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\a'); return true; }
#line 5182 "src/parse/lex.cc"
yy791:
	++cur;
#line 825 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\b'); return true; }
#line 5187 "src/parse/lex.cc"
yy793:
	++cur;
#line 826 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\f'); return true; }
#line 5192 "src/parse/lex.cc"
yy795:
	++cur;
#line 827 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\n'); return true; }
#line 5197 "src/parse/lex.cc"
yy797:
	++cur;
#line 828 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\r'); return true; }
#line 5202 "src/parse/lex.cc"
yy799:
	++cur;
#line 829 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\t'); return true; }
#line 5207 "src/parse/lex.cc"
yy801:
	++cur;
#line 830 "../src/parse/lex.re"
	{ ast.chr = static_cast<uint8_t>('\v'); return true; }
#line 5212 "src/parse/lex.cc"
yy803:
	yyaccept = 3;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy785;
		if (yych <= '9') goto yy816;
		goto yy785;
	} else {
		if (yych <= 'F') goto yy816;
		if (yych <= '`') goto yy785;
		if (yych <= 'f') goto yy816;
		goto yy785;
	}
yy804:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy805;
	if (yych <= 0xBF) goto yy778;
yy805:
	cur = mar;
	if (yyaccept <= 1) {
		if (yyaccept == 0) {
			goto yy769;
		} else {
			goto yy771;
		}
	} else {
		if (yyaccept == 2) {
			goto yy782;
		} else {
			goto yy785;
		}
	}
yy806:
	yych = (unsigned char)*++cur;
	if (yych <= 0x9F) goto yy805;
	if (yych <= 0xBF) goto yy804;
	goto yy805;
yy807:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy805;
	if (yych <= 0xBF) goto yy804;
	goto yy805;
yy808:
	yych = (unsigned char)*++cur;
	if (yych <= 0x8F) goto yy805;
	if (yych <= 0xBF) goto yy807;
	goto yy805;
yy809:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy805;
	if (yych <= 0xBF) goto yy807;
	goto yy805;
yy810:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy805;
	if (yych <= 0x8F) goto yy807;
	goto yy805;
yy811:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy805;
	if (yych <= 0xBF) goto yy763;
	goto yy805;
yy812:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy805;
	if (yych <= 0xBF) goto yy811;
	goto yy805;
yy813:
	yych = (unsigned char)*++cur;
	if (yych <= '/') goto yy805;
	if (yych <= '7') goto yy817;
	goto yy805;
yy814:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy805;
		if (yych <= '9') goto yy819;
		goto yy805;
	} else {
		if (yych <= 'F') goto yy819;
		if (yych <= '`') goto yy805;
		if (yych <= 'f') goto yy819;
		goto yy805;
	}
yy815:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy805;
		if (yych <= '9') goto yy820;
		goto yy805;
	} else {
		if (yych <= 'F') goto yy820;
		if (yych <= '`') goto yy805;
		if (yych <= 'f') goto yy820;
		goto yy805;
	}
yy816:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy805;
		if (yych <= '9') goto yy821;
		goto yy805;
	} else {
		if (yych <= 'F') goto yy821;
		if (yych <= '`') goto yy805;
		if (yych <= 'f') goto yy821;
		goto yy805;
	}
yy817:
	++cur;
#line 823 "../src/parse/lex.re"
	{ ast.chr = unesc_oct(tok, cur); return true; }
#line 5325 "src/parse/lex.cc"
yy819:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy805;
		if (yych <= '9') goto yy823;
		goto yy805;
	} else {
		if (yych <= 'F') goto yy823;
		if (yych <= '`') goto yy805;
		if (yych <= 'f') goto yy823;
		goto yy805;
	}
yy820:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy805;
		if (yych <= '9') goto yy816;
		goto yy805;
	} else {
		if (yych <= 'F') goto yy816;
		if (yych <= '`') goto yy805;
		if (yych <= 'f') goto yy816;
		goto yy805;
	}
yy821:
	++cur;
#line 822 "../src/parse/lex.re"
	{ ast.chr = unesc_hex(tok, cur); return true; }
#line 5354 "src/parse/lex.cc"
yy823:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy805;
		if (yych >= ':') goto yy805;
	} else {
		if (yych <= 'F') goto yy824;
		if (yych <= '`') goto yy805;
		if (yych >= 'g') goto yy805;
	}
yy824:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy805;
		if (yych <= '9') goto yy815;
		goto yy805;
	} else {
		if (yych <= 'F') goto yy815;
		if (yych <= '`') goto yy805;
		if (yych <= 'f') goto yy815;
		goto yy805;
	}
}
#line 842 "../src/parse/lex.re"

    }
}