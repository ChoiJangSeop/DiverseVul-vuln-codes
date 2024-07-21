static js_Ast *shift(js_State *J)
{
	js_Ast *a = additive(J);
loop:
	if (jsP_accept(J, TK_SHL)) { a = EXP2(SHL, a, additive(J)); goto loop; }
	if (jsP_accept(J, TK_SHR)) { a = EXP2(SHR, a, additive(J)); goto loop; }
	if (jsP_accept(J, TK_USHR)) { a = EXP2(USHR, a, additive(J)); goto loop; }
	return a;
}