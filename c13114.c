find_abbrev_list_by_abbrev_offset (dwarf_vma abbrev_base,
				   dwarf_vma abbrev_offset)
{
  abbrev_list * list;

  for (list = abbrev_lists; list != NULL; list = list->next)
    if (list->abbrev_base == abbrev_base
	&& list->abbrev_offset == abbrev_offset)
      return list;

  return NULL;
}