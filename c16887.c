bool Item::cache_const_expr_analyzer(uchar **arg)
{
  bool *cache_flag= (bool*)*arg;
  if (!*cache_flag)
  {
    Item *item= real_item();
    /*
      Cache constant items unless it's a basic constant, constant field or
      a subselect (they use their own cache).
    */
    if (const_item() &&
        !(basic_const_item() || item->basic_const_item() ||
          item->type() == Item::FIELD_ITEM ||
          item->type() == SUBSELECT_ITEM ||
           /*
             Do not cache GET_USER_VAR() function as its const_item() may
             return TRUE for the current thread but it still may change
             during the execution.
           */
          (item->type() == Item::FUNC_ITEM &&
           ((Item_func*)item)->functype() == Item_func::GUSERVAR_FUNC)))
      *cache_flag= TRUE;
    return TRUE;
  }
  return FALSE;
}