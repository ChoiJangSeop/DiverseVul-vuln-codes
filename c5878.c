static js_Ast *multiplicative(js_State *J)
{
	js_Ast *a = unary(J);
loop:
	if (jsP_accept(J, '*')) { a = EXP2(MUL, a, unary(J)); goto loop; }
	if (jsP_accept(J, '/')) { a = EXP2(DIV, a, unary(J)); goto loop; }
	if (jsP_accept(J, '%')) { a = EXP2(MOD, a, unary(J)); goto loop; }
	return a;
}