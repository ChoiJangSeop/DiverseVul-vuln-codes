void jsP_dumplist(js_State *J, js_Ast *prog)
{
	minify = 0;
	if (prog->type == AST_LIST)
		sblock(0, prog);
	else
		snode(0, prog);
	nl();
}