Item* Item_singlerow_subselect::expr_cache_insert_transformer(THD *tmp_thd,
                                                              uchar *unused)
{
  DBUG_ENTER("Item_singlerow_subselect::expr_cache_insert_transformer");

  DBUG_ASSERT(thd == tmp_thd);

  if (expr_cache)
    DBUG_RETURN(expr_cache);

  if (expr_cache_is_needed(tmp_thd) &&
      (expr_cache= set_expr_cache(tmp_thd)))
  {
    init_expr_cache_tracker(tmp_thd);
    DBUG_RETURN(expr_cache);
  }
  DBUG_RETURN(this);
}