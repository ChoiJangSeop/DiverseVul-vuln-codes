uint32_t Scanner::lex_cls_chr()
{
    tok = cur;
    const loc_t &loc = cur_loc();
    #line 798 "../src/parse/lex.re"

    if (globopts->input_encoding == Enc::ASCII) {
        
#line 3869 "src/parse/lex.cc"
{
	unsigned char yych;
	unsigned int yyaccept = 0;
	if ((lim - cur) < 10) { if (!fill(10)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= '\f') {
		if (yych <= 0x00) goto yy587;
		if (yych == '\n') goto yy591;
		goto yy589;
	} else {
		if (yych <= '\r') goto yy593;
		if (yych == '\\') goto yy594;
		goto yy589;
	}
yy587:
	++cur;
#line 779 "../src/parse/lex.re"
	{ fail_if_eof(); return 0; }
#line 3888 "src/parse/lex.cc"
yy589:
	++cur;
yy590:
#line 781 "../src/parse/lex.re"
	{ return decode(tok); }
#line 3894 "src/parse/lex.cc"
yy591:
	++cur;
#line 773 "../src/parse/lex.re"
	{ msg.error(loc, "newline in character class"); exit(1); }
#line 3899 "src/parse/lex.cc"
yy593:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy591;
	goto yy590;
yy594:
	yych = (unsigned char)*++cur;
	if (yych <= '\\') {
		if (yych <= '/') {
			if (yych <= '\f') {
				if (yych <= 0x00) goto yy595;
				if (yych == '\n') goto yy591;
				goto yy596;
			} else {
				if (yych <= '\r') goto yy598;
				if (yych == '-') goto yy599;
				goto yy596;
			}
		} else {
			if (yych <= 'U') {
				if (yych <= '3') goto yy601;
				if (yych <= '7') goto yy603;
				if (yych <= 'T') goto yy596;
				goto yy604;
			} else {
				if (yych == 'X') goto yy606;
				if (yych <= '[') goto yy596;
				goto yy607;
			}
		}
	} else {
		if (yych <= 'n') {
			if (yych <= 'b') {
				if (yych <= ']') goto yy609;
				if (yych <= '`') goto yy596;
				if (yych <= 'a') goto yy611;
				goto yy613;
			} else {
				if (yych == 'f') goto yy615;
				if (yych <= 'm') goto yy596;
				goto yy617;
			}
		} else {
			if (yych <= 't') {
				if (yych == 'r') goto yy619;
				if (yych <= 's') goto yy596;
				goto yy621;
			} else {
				if (yych <= 'v') {
					if (yych <= 'u') goto yy606;
					goto yy623;
				} else {
					if (yych == 'x') goto yy625;
					goto yy596;
				}
			}
		}
	}
yy595:
#line 776 "../src/parse/lex.re"
	{ msg.error(loc, "syntax error in escape sequence"); exit(1); }
#line 3960 "src/parse/lex.cc"
yy596:
	++cur;
yy597:
#line 794 "../src/parse/lex.re"
	{
        msg.warn.useless_escape(loc, tok, cur);
        return decode(tok + 1);
    }
#line 3969 "src/parse/lex.cc"
yy598:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy591;
	goto yy597;
yy599:
	++cur;
#line 792 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('-'); }
#line 3978 "src/parse/lex.cc"
yy601:
	yyaccept = 0;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '/') goto yy602;
	if (yych <= '7') goto yy626;
yy602:
#line 775 "../src/parse/lex.re"
	{ msg.error(loc, "syntax error in octal escape sequence"); exit(1); }
#line 3987 "src/parse/lex.cc"
yy603:
	++cur;
	goto yy602;
yy604:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy605;
		if (yych <= '9') goto yy628;
	} else {
		if (yych <= 'F') goto yy628;
		if (yych <= '`') goto yy605;
		if (yych <= 'f') goto yy628;
	}
yy605:
#line 774 "../src/parse/lex.re"
	{ msg.error(loc, "syntax error in hexadecimal escape sequence"); exit(1); }
#line 4005 "src/parse/lex.cc"
yy606:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy605;
		if (yych <= '9') goto yy629;
		goto yy605;
	} else {
		if (yych <= 'F') goto yy629;
		if (yych <= '`') goto yy605;
		if (yych <= 'f') goto yy629;
		goto yy605;
	}
yy607:
	++cur;
#line 791 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\\'); }
#line 4023 "src/parse/lex.cc"
yy609:
	++cur;
#line 793 "../src/parse/lex.re"
	{ return static_cast<uint8_t>(']'); }
#line 4028 "src/parse/lex.cc"
yy611:
	++cur;
#line 784 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\a'); }
#line 4033 "src/parse/lex.cc"
yy613:
	++cur;
#line 785 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\b'); }
#line 4038 "src/parse/lex.cc"
yy615:
	++cur;
#line 786 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\f'); }
#line 4043 "src/parse/lex.cc"
yy617:
	++cur;
#line 787 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\n'); }
#line 4048 "src/parse/lex.cc"
yy619:
	++cur;
#line 788 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\r'); }
#line 4053 "src/parse/lex.cc"
yy621:
	++cur;
#line 789 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\t'); }
#line 4058 "src/parse/lex.cc"
yy623:
	++cur;
#line 790 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\v'); }
#line 4063 "src/parse/lex.cc"
yy625:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy605;
		if (yych <= '9') goto yy630;
		goto yy605;
	} else {
		if (yych <= 'F') goto yy630;
		if (yych <= '`') goto yy605;
		if (yych <= 'f') goto yy630;
		goto yy605;
	}
yy626:
	yych = (unsigned char)*++cur;
	if (yych <= '/') goto yy627;
	if (yych <= '7') goto yy631;
yy627:
	cur = mar;
	if (yyaccept == 0) {
		goto yy602;
	} else {
		goto yy605;
	}
yy628:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy627;
		if (yych <= '9') goto yy633;
		goto yy627;
	} else {
		if (yych <= 'F') goto yy633;
		if (yych <= '`') goto yy627;
		if (yych <= 'f') goto yy633;
		goto yy627;
	}
yy629:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy627;
		if (yych <= '9') goto yy634;
		goto yy627;
	} else {
		if (yych <= 'F') goto yy634;
		if (yych <= '`') goto yy627;
		if (yych <= 'f') goto yy634;
		goto yy627;
	}
yy630:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy627;
		if (yych <= '9') goto yy635;
		goto yy627;
	} else {
		if (yych <= 'F') goto yy635;
		if (yych <= '`') goto yy627;
		if (yych <= 'f') goto yy635;
		goto yy627;
	}
yy631:
	++cur;
#line 783 "../src/parse/lex.re"
	{ return unesc_oct(tok, cur); }
#line 4128 "src/parse/lex.cc"
yy633:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy627;
		if (yych <= '9') goto yy637;
		goto yy627;
	} else {
		if (yych <= 'F') goto yy637;
		if (yych <= '`') goto yy627;
		if (yych <= 'f') goto yy637;
		goto yy627;
	}
yy634:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy627;
		if (yych <= '9') goto yy630;
		goto yy627;
	} else {
		if (yych <= 'F') goto yy630;
		if (yych <= '`') goto yy627;
		if (yych <= 'f') goto yy630;
		goto yy627;
	}
yy635:
	++cur;
#line 782 "../src/parse/lex.re"
	{ return unesc_hex(tok, cur); }
#line 4157 "src/parse/lex.cc"
yy637:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy627;
		if (yych >= ':') goto yy627;
	} else {
		if (yych <= 'F') goto yy638;
		if (yych <= '`') goto yy627;
		if (yych >= 'g') goto yy627;
	}
yy638:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy627;
		if (yych <= '9') goto yy629;
		goto yy627;
	} else {
		if (yych <= 'F') goto yy629;
		if (yych <= '`') goto yy627;
		if (yych <= 'f') goto yy629;
		goto yy627;
	}
}
#line 800 "../src/parse/lex.re"

    }
    else {
        
#line 4186 "src/parse/lex.cc"
{
	unsigned char yych;
	unsigned int yyaccept = 0;
	if ((lim - cur) < 10) { if (!fill(10)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych <= 0x7F) {
		if (yych <= '\f') {
			if (yych <= 0x00) goto yy641;
			if (yych == '\n') goto yy645;
			goto yy643;
		} else {
			if (yych <= '\r') goto yy647;
			if (yych == '\\') goto yy648;
			goto yy643;
		}
	} else {
		if (yych <= 0xEF) {
			if (yych <= 0xC1) goto yy650;
			if (yych <= 0xDF) goto yy652;
			if (yych <= 0xE0) goto yy653;
			goto yy654;
		} else {
			if (yych <= 0xF0) goto yy655;
			if (yych <= 0xF3) goto yy656;
			if (yych <= 0xF4) goto yy657;
			goto yy650;
		}
	}
yy641:
	++cur;
#line 779 "../src/parse/lex.re"
	{ fail_if_eof(); return 0; }
#line 4219 "src/parse/lex.cc"
yy643:
	++cur;
yy644:
#line 781 "../src/parse/lex.re"
	{ return decode(tok); }
#line 4225 "src/parse/lex.cc"
yy645:
	++cur;
#line 773 "../src/parse/lex.re"
	{ msg.error(loc, "newline in character class"); exit(1); }
#line 4230 "src/parse/lex.cc"
yy647:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy645;
	goto yy644;
yy648:
	yyaccept = 0;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 'b') {
		if (yych <= '7') {
			if (yych <= '\r') {
				if (yych <= '\t') {
					if (yych >= 0x01) goto yy658;
				} else {
					if (yych <= '\n') goto yy645;
					if (yych <= '\f') goto yy658;
					goto yy660;
				}
			} else {
				if (yych <= '-') {
					if (yych <= ',') goto yy658;
					goto yy661;
				} else {
					if (yych <= '/') goto yy658;
					if (yych <= '3') goto yy663;
					goto yy665;
				}
			}
		} else {
			if (yych <= '[') {
				if (yych <= 'U') {
					if (yych <= 'T') goto yy658;
					goto yy666;
				} else {
					if (yych == 'X') goto yy668;
					goto yy658;
				}
			} else {
				if (yych <= ']') {
					if (yych <= '\\') goto yy669;
					goto yy671;
				} else {
					if (yych <= '`') goto yy658;
					if (yych <= 'a') goto yy673;
					goto yy675;
				}
			}
		}
	} else {
		if (yych <= 'v') {
			if (yych <= 'q') {
				if (yych <= 'f') {
					if (yych <= 'e') goto yy658;
					goto yy677;
				} else {
					if (yych == 'n') goto yy679;
					goto yy658;
				}
			} else {
				if (yych <= 's') {
					if (yych <= 'r') goto yy681;
					goto yy658;
				} else {
					if (yych <= 't') goto yy683;
					if (yych <= 'u') goto yy668;
					goto yy685;
				}
			}
		} else {
			if (yych <= 0xDF) {
				if (yych <= 'x') {
					if (yych <= 'w') goto yy658;
					goto yy687;
				} else {
					if (yych <= 0x7F) goto yy658;
					if (yych >= 0xC2) goto yy688;
				}
			} else {
				if (yych <= 0xF0) {
					if (yych <= 0xE0) goto yy690;
					if (yych <= 0xEF) goto yy691;
					goto yy692;
				} else {
					if (yych <= 0xF3) goto yy693;
					if (yych <= 0xF4) goto yy694;
				}
			}
		}
	}
yy649:
#line 776 "../src/parse/lex.re"
	{ msg.error(loc, "syntax error in escape sequence"); exit(1); }
#line 4322 "src/parse/lex.cc"
yy650:
	++cur;
yy651:
#line 777 "../src/parse/lex.re"
	{ msg.error(loc, "syntax error"); exit(1); }
#line 4328 "src/parse/lex.cc"
yy652:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy651;
	if (yych <= 0xBF) goto yy643;
	goto yy651;
yy653:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x9F) goto yy651;
	if (yych <= 0xBF) goto yy695;
	goto yy651;
yy654:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x7F) goto yy651;
	if (yych <= 0xBF) goto yy695;
	goto yy651;
yy655:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x8F) goto yy651;
	if (yych <= 0xBF) goto yy696;
	goto yy651;
yy656:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x7F) goto yy651;
	if (yych <= 0xBF) goto yy696;
	goto yy651;
yy657:
	yyaccept = 1;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= 0x7F) goto yy651;
	if (yych <= 0x8F) goto yy696;
	goto yy651;
yy658:
	++cur;
yy659:
#line 794 "../src/parse/lex.re"
	{
        msg.warn.useless_escape(loc, tok, cur);
        return decode(tok + 1);
    }
#line 4372 "src/parse/lex.cc"
yy660:
	yych = (unsigned char)*++cur;
	if (yych == '\n') goto yy645;
	goto yy659;
yy661:
	++cur;
#line 792 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('-'); }
#line 4381 "src/parse/lex.cc"
yy663:
	yyaccept = 2;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '/') goto yy664;
	if (yych <= '7') goto yy697;
yy664:
#line 775 "../src/parse/lex.re"
	{ msg.error(loc, "syntax error in octal escape sequence"); exit(1); }
#line 4390 "src/parse/lex.cc"
yy665:
	++cur;
	goto yy664;
yy666:
	yyaccept = 3;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy667;
		if (yych <= '9') goto yy698;
	} else {
		if (yych <= 'F') goto yy698;
		if (yych <= '`') goto yy667;
		if (yych <= 'f') goto yy698;
	}
yy667:
#line 774 "../src/parse/lex.re"
	{ msg.error(loc, "syntax error in hexadecimal escape sequence"); exit(1); }
#line 4408 "src/parse/lex.cc"
yy668:
	yyaccept = 3;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy667;
		if (yych <= '9') goto yy699;
		goto yy667;
	} else {
		if (yych <= 'F') goto yy699;
		if (yych <= '`') goto yy667;
		if (yych <= 'f') goto yy699;
		goto yy667;
	}
yy669:
	++cur;
#line 791 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\\'); }
#line 4426 "src/parse/lex.cc"
yy671:
	++cur;
#line 793 "../src/parse/lex.re"
	{ return static_cast<uint8_t>(']'); }
#line 4431 "src/parse/lex.cc"
yy673:
	++cur;
#line 784 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\a'); }
#line 4436 "src/parse/lex.cc"
yy675:
	++cur;
#line 785 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\b'); }
#line 4441 "src/parse/lex.cc"
yy677:
	++cur;
#line 786 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\f'); }
#line 4446 "src/parse/lex.cc"
yy679:
	++cur;
#line 787 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\n'); }
#line 4451 "src/parse/lex.cc"
yy681:
	++cur;
#line 788 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\r'); }
#line 4456 "src/parse/lex.cc"
yy683:
	++cur;
#line 789 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\t'); }
#line 4461 "src/parse/lex.cc"
yy685:
	++cur;
#line 790 "../src/parse/lex.re"
	{ return static_cast<uint8_t>('\v'); }
#line 4466 "src/parse/lex.cc"
yy687:
	yyaccept = 3;
	yych = (unsigned char)*(mar = ++cur);
	if (yych <= '@') {
		if (yych <= '/') goto yy667;
		if (yych <= '9') goto yy700;
		goto yy667;
	} else {
		if (yych <= 'F') goto yy700;
		if (yych <= '`') goto yy667;
		if (yych <= 'f') goto yy700;
		goto yy667;
	}
yy688:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy689;
	if (yych <= 0xBF) goto yy658;
yy689:
	cur = mar;
	if (yyaccept <= 1) {
		if (yyaccept == 0) {
			goto yy649;
		} else {
			goto yy651;
		}
	} else {
		if (yyaccept == 2) {
			goto yy664;
		} else {
			goto yy667;
		}
	}
yy690:
	yych = (unsigned char)*++cur;
	if (yych <= 0x9F) goto yy689;
	if (yych <= 0xBF) goto yy688;
	goto yy689;
yy691:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy689;
	if (yych <= 0xBF) goto yy688;
	goto yy689;
yy692:
	yych = (unsigned char)*++cur;
	if (yych <= 0x8F) goto yy689;
	if (yych <= 0xBF) goto yy691;
	goto yy689;
yy693:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy689;
	if (yych <= 0xBF) goto yy691;
	goto yy689;
yy694:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy689;
	if (yych <= 0x8F) goto yy691;
	goto yy689;
yy695:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy689;
	if (yych <= 0xBF) goto yy643;
	goto yy689;
yy696:
	yych = (unsigned char)*++cur;
	if (yych <= 0x7F) goto yy689;
	if (yych <= 0xBF) goto yy695;
	goto yy689;
yy697:
	yych = (unsigned char)*++cur;
	if (yych <= '/') goto yy689;
	if (yych <= '7') goto yy701;
	goto yy689;
yy698:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy689;
		if (yych <= '9') goto yy703;
		goto yy689;
	} else {
		if (yych <= 'F') goto yy703;
		if (yych <= '`') goto yy689;
		if (yych <= 'f') goto yy703;
		goto yy689;
	}
yy699:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy689;
		if (yych <= '9') goto yy704;
		goto yy689;
	} else {
		if (yych <= 'F') goto yy704;
		if (yych <= '`') goto yy689;
		if (yych <= 'f') goto yy704;
		goto yy689;
	}
yy700:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy689;
		if (yych <= '9') goto yy705;
		goto yy689;
	} else {
		if (yych <= 'F') goto yy705;
		if (yych <= '`') goto yy689;
		if (yych <= 'f') goto yy705;
		goto yy689;
	}
yy701:
	++cur;
#line 783 "../src/parse/lex.re"
	{ return unesc_oct(tok, cur); }
#line 4579 "src/parse/lex.cc"
yy703:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy689;
		if (yych <= '9') goto yy707;
		goto yy689;
	} else {
		if (yych <= 'F') goto yy707;
		if (yych <= '`') goto yy689;
		if (yych <= 'f') goto yy707;
		goto yy689;
	}
yy704:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy689;
		if (yych <= '9') goto yy700;
		goto yy689;
	} else {
		if (yych <= 'F') goto yy700;
		if (yych <= '`') goto yy689;
		if (yych <= 'f') goto yy700;
		goto yy689;
	}
yy705:
	++cur;
#line 782 "../src/parse/lex.re"
	{ return unesc_hex(tok, cur); }
#line 4608 "src/parse/lex.cc"
yy707:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy689;
		if (yych >= ':') goto yy689;
	} else {
		if (yych <= 'F') goto yy708;
		if (yych <= '`') goto yy689;
		if (yych >= 'g') goto yy689;
	}
yy708:
	yych = (unsigned char)*++cur;
	if (yych <= '@') {
		if (yych <= '/') goto yy689;
		if (yych <= '9') goto yy699;
		goto yy689;
	} else {
		if (yych <= 'F') goto yy699;
		if (yych <= '`') goto yy689;
		if (yych <= 'f') goto yy699;
		goto yy689;
	}
}
#line 803 "../src/parse/lex.re"

    }
}