static js_Ast *callexp(js_State *J)
{
	js_Ast *a = newexp(J);
loop:
	if (jsP_accept(J, '.')) { a = EXP2(MEMBER, a, identifiername(J)); goto loop; }
	if (jsP_accept(J, '[')) { a = EXP2(INDEX, a, expression(J, 0)); jsP_expect(J, ']'); goto loop; }
	if (jsP_accept(J, '(')) { a = EXP2(CALL, a, arguments(J)); jsP_expect(J, ')'); goto loop; }
	return a;
}