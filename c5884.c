static js_Ast *expression(js_State *J, int notin)
{
	js_Ast *a;
	INCREC();
	a = assignment(J, notin);
	while (jsP_accept(J, ','))
		a = EXP2(COMMA, a, assignment(J, notin));
	DECREC();
	return a;
}