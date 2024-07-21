int compare_order_lists(SQL_I_List<ORDER> *part_list1,
                        SQL_I_List<ORDER> *part_list2)
{
  if (part_list1 == part_list2)
    return CMP_EQ;
  ORDER *elem1= part_list1->first;
  ORDER *elem2= part_list2->first;
  for ( ; elem1 && elem2; elem1= elem1->next, elem2= elem2->next)
  {
    int cmp;
    // remove all constants as we don't need them for comparision
    while(elem1 && ((*elem1->item)->real_item())->const_item())
    {
      elem1= elem1->next;
      continue;
    }

    while(elem2 && ((*elem2->item)->real_item())->const_item())
    {
      elem2= elem2->next;
      continue;
    }

    if (!elem1 || !elem2)
      break;

    if ((cmp= compare_order_elements(elem1, elem2)))
      return cmp;
  }
  if (elem1)
    return CMP_GT_C;
  if (elem2)
    return CMP_LT_C;
  return CMP_EQ;
}