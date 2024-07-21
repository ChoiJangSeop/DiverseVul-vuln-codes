static js_Ast *conditional(js_State *J, int notin)
{
	js_Ast *a, *b, *c;
	a = logor(J, notin);
	if (jsP_accept(J, '?')) {
		b = assignment(J, 0);
		jsP_expect(J, ':');
		c = assignment(J, notin);
		return EXP3(COND, a, b, c);
	}
	return a;
}