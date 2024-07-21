static js_Ast *additive(js_State *J)
{
	js_Ast *a = multiplicative(J);
loop:
	if (jsP_accept(J, '+')) { a = EXP2(ADD, a, multiplicative(J)); goto loop; }
	if (jsP_accept(J, '-')) { a = EXP2(SUB, a, multiplicative(J)); goto loop; }
	return a;
}