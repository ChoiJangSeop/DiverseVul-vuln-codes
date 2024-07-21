static js_Ast *relational(js_State *J, int notin)
{
	js_Ast *a = shift(J);
loop:
	if (jsP_accept(J, '<')) { a = EXP2(LT, a, shift(J)); goto loop; }
	if (jsP_accept(J, '>')) { a = EXP2(GT, a, shift(J)); goto loop; }
	if (jsP_accept(J, TK_LE)) { a = EXP2(LE, a, shift(J)); goto loop; }
	if (jsP_accept(J, TK_GE)) { a = EXP2(GE, a, shift(J)); goto loop; }
	if (jsP_accept(J, TK_INSTANCEOF)) { a = EXP2(INSTANCEOF, a, shift(J)); goto loop; }
	if (!notin && jsP_accept(J, TK_IN)) { a = EXP2(IN, a, shift(J)); goto loop; }
	return a;
}