static js_Ast *equality(js_State *J, int notin)
{
	js_Ast *a = relational(J, notin);
loop:
	if (jsP_accept(J, TK_EQ)) { a = EXP2(EQ, a, relational(J, notin)); goto loop; }
	if (jsP_accept(J, TK_NE)) { a = EXP2(NE, a, relational(J, notin)); goto loop; }
	if (jsP_accept(J, TK_STRICTEQ)) { a = EXP2(STRICTEQ, a, relational(J, notin)); goto loop; }
	if (jsP_accept(J, TK_STRICTNE)) { a = EXP2(STRICTNE, a, relational(J, notin)); goto loop; }
	return a;
}