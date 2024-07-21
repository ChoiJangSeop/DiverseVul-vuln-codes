int compare_order_elements(ORDER *ord1, ORDER *ord2)
{
  if (*ord1->item == *ord2->item && ord1->direction == ord2->direction)
    return CMP_EQ;
  Item *item1= (*ord1->item)->real_item();
  Item *item2= (*ord2->item)->real_item();
  DBUG_ASSERT(item1->type() == Item::FIELD_ITEM &&
              item2->type() == Item::FIELD_ITEM); 
  ptrdiff_t cmp= ((Item_field *) item1)->field - ((Item_field *) item2)->field;
  if (cmp == 0)
  {
    if (ord1->direction == ord2->direction)
      return CMP_EQ;
    return ord1->direction > ord2->direction ? CMP_GT : CMP_LT;
  }
  else
    return cmp > 0 ? CMP_GT : CMP_LT;
}