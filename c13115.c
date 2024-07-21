find_and_process_abbrev_set (struct dwarf_section *section,
			     dwarf_vma abbrev_base,
			     dwarf_vma abbrev_size,
			     dwarf_vma abbrev_offset,
			     abbrev_list **free_list)
{
  if (free_list)
    *free_list = NULL;

  if (abbrev_base >= section->size
      || abbrev_size > section->size - abbrev_base)
    {
      /* PR 17531: file:4bcd9ce9.  */
      warn (_("Debug info is corrupted, abbrev size (%lx) is larger than "
	      "abbrev section size (%lx)\n"),
	      (unsigned long) (abbrev_base + abbrev_size),
	      (unsigned long) section->size);
      return NULL;
    }
  if (abbrev_offset >= abbrev_size)
    {
      warn (_("Debug info is corrupted, abbrev offset (%lx) is larger than "
	      "abbrev section size (%lx)\n"),
	    (unsigned long) abbrev_offset,
	    (unsigned long) abbrev_size);
      return NULL;
    }

  unsigned char *start = section->start + abbrev_base + abbrev_offset;
  unsigned char *end = section->start + abbrev_base + abbrev_size;
  abbrev_list *list = NULL;
  if (free_list)
    list = find_abbrev_list_by_abbrev_offset (abbrev_base, abbrev_offset);
  if (list == NULL)
    {
      list = process_abbrev_set (section, start, end);
      if (list)
	{
	  list->abbrev_base = abbrev_base;
	  list->abbrev_offset = abbrev_offset;
	  list->next = NULL;
	}
      if (free_list)
	*free_list = list;
    }
  return list;
}