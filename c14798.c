const AST *ast_iter(const AST *a, uint32_t n, uint32_t m)
{
    AST *ast = new AST(a->loc, AST::ITER);
    ast->iter.ast = a;
    ast->iter.min = n;
    ast->iter.max = m;
    return ast;
}