process_abbrev_set (struct dwarf_section *section,
		    unsigned char *start,
		    unsigned char *end)
{
  abbrev_list *list = xmalloc (sizeof (*list));
  list->first_abbrev = NULL;
  list->last_abbrev = NULL;

  while (start < end)
    {
      unsigned long entry;
      unsigned long tag;
      unsigned long attribute;
      int children;

      READ_ULEB (entry, start, end);

      /* A single zero is supposed to end the set according
	 to the standard.  If there's more, then signal that to
	 the caller.  */
      if (start == end || entry == 0)
	{
	  list->start_of_next_abbrevs = start != end ? start : NULL;
	  return list;
	}

      READ_ULEB (tag, start, end);
      if (start == end)
	{
	  free (list);
	  return NULL;
	}

      children = *start++;

      add_abbrev (entry, tag, children, list);

      do
	{
	  unsigned long form;
	  /* Initialize it due to a false compiler warning.  */
	  dwarf_signed_vma implicit_const = -1;

	  READ_ULEB (attribute, start, end);
	  if (start == end)
	    break;

	  READ_ULEB (form, start, end);
	  if (start == end)
	    break;

	  if (form == DW_FORM_implicit_const)
	    {
	      READ_SLEB (implicit_const, start, end);
	      if (start == end)
		break;
	    }

	  add_abbrev_attr (attribute, form, implicit_const, list);
	}
      while (attribute != 0);
    }

  /* Report the missing single zero which ends the section.  */
  error (_("%s section not zero terminated\n"), section->name);

  free (list);
  return NULL;
}