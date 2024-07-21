struct nft_expr *nft_set_elem_expr_alloc(const struct nft_ctx *ctx,
					 const struct nft_set *set,
					 const struct nlattr *attr)
{
	struct nft_expr *expr;
	int err;

	expr = nft_expr_init(ctx, attr);
	if (IS_ERR(expr))
		return expr;

	err = -EOPNOTSUPP;
	if (!(expr->ops->type->flags & NFT_EXPR_STATEFUL))
		goto err_set_elem_expr;

	if (expr->ops->type->flags & NFT_EXPR_GC) {
		if (set->flags & NFT_SET_TIMEOUT)
			goto err_set_elem_expr;
		if (!set->ops->gc_init)
			goto err_set_elem_expr;
		set->ops->gc_init(set);
	}

	return expr;

err_set_elem_expr:
	nft_expr_destroy(ctx, expr);
	return ERR_PTR(err);
}