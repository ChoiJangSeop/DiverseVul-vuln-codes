const AST *Scanner::lex_cls(bool neg)
{
    std::vector<ASTRange> *cls = new std::vector<ASTRange>;
    uint32_t u, l;
    const loc_t &loc0 = tok_loc();
    loc_t loc = cur_loc();
fst:
    tok = cur;

#line 3805 "src/parse/lex.cc"
{
	unsigned char yych;
	if (lim <= cur) { if (!fill(1)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*cur;
	if (yych == ']') goto yy576;
#line 747 "../src/parse/lex.re"
	{ l = lex_cls_chr(); goto snd; }
#line 3813 "src/parse/lex.cc"
yy576:
	++cur;
#line 746 "../src/parse/lex.re"
	{ return ast_cls(loc0, cls, neg); }
#line 3818 "src/parse/lex.cc"
}
#line 748 "../src/parse/lex.re"

snd:

#line 3824 "src/parse/lex.cc"
{
	unsigned char yych;
	if ((lim - cur) < 2) { if (!fill(2)) { error("unexpected end of input"); exit(1); } }
	yych = (unsigned char)*(mar = cur);
	if (yych == '-') goto yy581;
yy580:
#line 751 "../src/parse/lex.re"
	{ u = l; goto add; }
#line 3833 "src/parse/lex.cc"
yy581:
	yych = (unsigned char)*++cur;
	if (yych != ']') goto yy583;
	cur = mar;
	goto yy580;
yy583:
	++cur;
	cur -= 1;
#line 752 "../src/parse/lex.re"
	{
        u = lex_cls_chr();
        if (l > u) {
            msg.warn.swapped_range(loc, l, u);
            std::swap(l, u);
        }
        goto add;
    }
#line 3851 "src/parse/lex.cc"
}
#line 760 "../src/parse/lex.re"

add:
    cls->push_back(ASTRange(l, u, loc));
    loc = cur_loc();
    goto fst;
}