display_debug_names (struct dwarf_section *section, void *file)
{
  unsigned char *hdrptr = section->start;
  dwarf_vma unit_length;
  unsigned char *unit_start;
  const unsigned char *const section_end = section->start + section->size;
  unsigned char *unit_end;

  introduce (section, false);

  load_debug_section_with_follow (str, file);

  for (; hdrptr < section_end; hdrptr = unit_end)
    {
      unsigned int offset_size;
      uint16_t dwarf_version, padding;
      uint32_t comp_unit_count, local_type_unit_count, foreign_type_unit_count;
      uint64_t bucket_count, name_count, abbrev_table_size;
      uint32_t augmentation_string_size;
      unsigned int i;
      bool augmentation_printable;
      const char *augmentation_string;
      size_t total;

      unit_start = hdrptr;

      /* Get and check the length of the block.  */
      SAFE_BYTE_GET_AND_INC (unit_length, hdrptr, 4, section_end);

      if (unit_length == 0xffffffff)
	{
	  /* This section is 64-bit DWARF.  */
	  SAFE_BYTE_GET_AND_INC (unit_length, hdrptr, 8, section_end);
	  offset_size = 8;
	}
      else
	offset_size = 4;

      if (unit_length > (size_t) (section_end - hdrptr)
	  || unit_length < 2 + 2 + 4 * 7)
	{
	too_short:
	  warn (_("Debug info is corrupted, %s header at %#lx has length %s\n"),
		section->name,
		(unsigned long) (unit_start - section->start),
		dwarf_vmatoa ("x", unit_length));
	  return 0;
	}
      unit_end = hdrptr + unit_length;

      /* Get and check the version number.  */
      SAFE_BYTE_GET_AND_INC (dwarf_version, hdrptr, 2, unit_end);
      printf (_("Version %ld\n"), (long) dwarf_version);

      /* Prior versions did not exist, and future versions may not be
	 backwards compatible.  */
      if (dwarf_version != 5)
	{
	  warn (_("Only DWARF version 5 .debug_names "
		  "is currently supported.\n"));
	  return 0;
	}

      SAFE_BYTE_GET_AND_INC (padding, hdrptr, 2, unit_end);
      if (padding != 0)
	warn (_("Padding field of .debug_names must be 0 (found 0x%x)\n"),
	      padding);

      SAFE_BYTE_GET_AND_INC (comp_unit_count, hdrptr, 4, unit_end);
      if (comp_unit_count == 0)
	warn (_("Compilation unit count must be >= 1 in .debug_names\n"));

      SAFE_BYTE_GET_AND_INC (local_type_unit_count, hdrptr, 4, unit_end);
      SAFE_BYTE_GET_AND_INC (foreign_type_unit_count, hdrptr, 4, unit_end);
      SAFE_BYTE_GET_AND_INC (bucket_count, hdrptr, 4, unit_end);
      SAFE_BYTE_GET_AND_INC (name_count, hdrptr, 4, unit_end);
      SAFE_BYTE_GET_AND_INC (abbrev_table_size, hdrptr, 4, unit_end);

      SAFE_BYTE_GET_AND_INC (augmentation_string_size, hdrptr, 4, unit_end);
      if (augmentation_string_size % 4 != 0)
	{
	  warn (_("Augmentation string length %u must be rounded up "
		  "to a multiple of 4 in .debug_names.\n"),
		augmentation_string_size);
	  augmentation_string_size += (-augmentation_string_size) & 3;
	}
      if (augmentation_string_size > (size_t) (unit_end - hdrptr))
	goto too_short;

      printf (_("Augmentation string:"));

      augmentation_printable = true;
      augmentation_string = (const char *) hdrptr;

      for (i = 0; i < augmentation_string_size; i++)
	{
	  unsigned char uc;

	  SAFE_BYTE_GET_AND_INC (uc, hdrptr, 1, unit_end);
	  printf (" %02x", uc);

	  if (uc != 0 && !ISPRINT (uc))
	    augmentation_printable = false;
	}

      if (augmentation_printable)
	{
	  printf ("  (\"");
	  for (i = 0;
	       i < augmentation_string_size && augmentation_string[i];
	       ++i)
	    putchar (augmentation_string[i]);
	  printf ("\")");
	}
      putchar ('\n');

      printf (_("CU table:\n"));
      if (_mul_overflow (comp_unit_count, offset_size, &total)
	  || total > (size_t) (unit_end - hdrptr))
	goto too_short;
      for (i = 0; i < comp_unit_count; i++)
	{
	  uint64_t cu_offset;

	  SAFE_BYTE_GET_AND_INC (cu_offset, hdrptr, offset_size, unit_end);
	  printf (_("[%3u] 0x%lx\n"), i, (unsigned long) cu_offset);
	}
      putchar ('\n');

      printf (_("TU table:\n"));
      if (_mul_overflow (local_type_unit_count, offset_size, &total)
	  || total > (size_t) (unit_end - hdrptr))
	goto too_short;
      for (i = 0; i < local_type_unit_count; i++)
	{
	  uint64_t tu_offset;

	  SAFE_BYTE_GET_AND_INC (tu_offset, hdrptr, offset_size, unit_end);
	  printf (_("[%3u] 0x%lx\n"), i, (unsigned long) tu_offset);
	}
      putchar ('\n');

      printf (_("Foreign TU table:\n"));
      if (_mul_overflow (foreign_type_unit_count, 8, &total)
	  || total > (size_t) (unit_end - hdrptr))
	goto too_short;
      for (i = 0; i < foreign_type_unit_count; i++)
	{
	  uint64_t signature;

	  SAFE_BYTE_GET_AND_INC (signature, hdrptr, 8, unit_end);
	  printf (_("[%3u] "), i);
	  print_dwarf_vma (signature, 8);
	  putchar ('\n');
	}
      putchar ('\n');

      uint64_t xtra = (bucket_count * sizeof (uint32_t)
		       + name_count * (sizeof (uint32_t) + 2 * offset_size)
		       + abbrev_table_size);
      if (xtra > (size_t) (unit_end - hdrptr))
	{
	  warn (_("Entry pool offset (0x%lx) exceeds unit size 0x%lx "
		  "for unit 0x%lx in the debug_names\n"),
		(long) xtra,
		(long) (unit_end - unit_start),
		(long) (unit_start - section->start));
	  return 0;
	}
      const uint32_t *const hash_table_buckets = (uint32_t *) hdrptr;
      hdrptr += bucket_count * sizeof (uint32_t);
      const uint32_t *const hash_table_hashes = (uint32_t *) hdrptr;
      hdrptr += name_count * sizeof (uint32_t);
      unsigned char *const name_table_string_offsets = hdrptr;
      hdrptr += name_count * offset_size;
      unsigned char *const name_table_entry_offsets = hdrptr;
      hdrptr += name_count * offset_size;
      unsigned char *const abbrev_table = hdrptr;
      hdrptr += abbrev_table_size;
      const unsigned char *const abbrev_table_end = hdrptr;
      unsigned char *const entry_pool = hdrptr;

      size_t buckets_filled = 0;
      size_t bucketi;
      for (bucketi = 0; bucketi < bucket_count; bucketi++)
	{
	  const uint32_t bucket = hash_table_buckets[bucketi];

	  if (bucket != 0)
	    ++buckets_filled;
	}
      printf (ngettext ("Used %zu of %lu bucket.\n",
			"Used %zu of %lu buckets.\n",
			bucket_count),
	      buckets_filled, (unsigned long) bucket_count);

      if (bucket_count != 0)
	{
	  uint32_t hash_prev = 0;
	  size_t hash_clash_count = 0;
	  size_t longest_clash = 0;
	  size_t this_length = 0;
	  size_t hashi;
	  for (hashi = 0; hashi < name_count; hashi++)
	    {
	      const uint32_t hash_this = hash_table_hashes[hashi];

	      if (hashi > 0)
		{
		  if (hash_prev % bucket_count == hash_this % bucket_count)
		    {
		      ++hash_clash_count;
		      ++this_length;
		      longest_clash = MAX (longest_clash, this_length);
		    }
		  else
		    this_length = 0;
		}
	      hash_prev = hash_this;
	    }
	  printf (_("Out of %lu items there are %zu bucket clashes"
		    " (longest of %zu entries).\n"),
		  (unsigned long) name_count, hash_clash_count, longest_clash);
	  assert (name_count == buckets_filled + hash_clash_count);
	}

      struct abbrev_lookup_entry
      {
	dwarf_vma abbrev_tag;
	unsigned char *abbrev_lookup_ptr;
      };
      struct abbrev_lookup_entry *abbrev_lookup = NULL;
      size_t abbrev_lookup_used = 0;
      size_t abbrev_lookup_allocated = 0;

      unsigned char *abbrevptr = abbrev_table;
      for (;;)
	{
	  dwarf_vma abbrev_tag;

	  READ_ULEB (abbrev_tag, abbrevptr, abbrev_table_end);
	  if (abbrev_tag == 0)
	    break;
	  if (abbrev_lookup_used == abbrev_lookup_allocated)
	    {
	      abbrev_lookup_allocated = MAX (0x100,
					     abbrev_lookup_allocated * 2);
	      abbrev_lookup = xrealloc (abbrev_lookup,
					(abbrev_lookup_allocated
					 * sizeof (*abbrev_lookup)));
	    }
	  assert (abbrev_lookup_used < abbrev_lookup_allocated);
	  struct abbrev_lookup_entry *entry;
	  for (entry = abbrev_lookup;
	       entry < abbrev_lookup + abbrev_lookup_used;
	       entry++)
	    if (entry->abbrev_tag == abbrev_tag)
	      {
		warn (_("Duplicate abbreviation tag %lu "
			"in unit 0x%lx in the debug_names\n"),
		      (long) abbrev_tag, (long) (unit_start - section->start));
		break;
	      }
	  entry = &abbrev_lookup[abbrev_lookup_used++];
	  entry->abbrev_tag = abbrev_tag;
	  entry->abbrev_lookup_ptr = abbrevptr;

	  /* Skip DWARF tag.  */
	  SKIP_ULEB (abbrevptr, abbrev_table_end);
	  for (;;)
	    {
	      dwarf_vma xindex, form;

	      READ_ULEB (xindex, abbrevptr, abbrev_table_end);
	      READ_ULEB (form, abbrevptr, abbrev_table_end);
	      if (xindex == 0 && form == 0)
		break;
	    }
	}

      printf (_("\nSymbol table:\n"));
      uint32_t namei;
      for (namei = 0; namei < name_count; ++namei)
	{
	  uint64_t string_offset, entry_offset;
	  unsigned char *p;

	  p = name_table_string_offsets + namei * offset_size;
	  SAFE_BYTE_GET (string_offset, p, offset_size, unit_end);
	  p = name_table_entry_offsets + namei * offset_size;
	  SAFE_BYTE_GET (entry_offset, p, offset_size, unit_end);

	  printf ("[%3u] #%08x %s:", namei, hash_table_hashes[namei],
		  fetch_indirect_string (string_offset));

	  unsigned char *entryptr = entry_pool + entry_offset;

	  /* We need to scan first whether there is a single or multiple
	     entries.  TAGNO is -2 for the first entry, it is -1 for the
	     initial tag read of the second entry, then it becomes 0 for the
	     first entry for real printing etc.  */
	  int tagno = -2;
	  /* Initialize it due to a false compiler warning.  */
	  dwarf_vma second_abbrev_tag = -1;
	  for (;;)
	    {
	      dwarf_vma abbrev_tag;
	      dwarf_vma dwarf_tag;
	      const struct abbrev_lookup_entry *entry;

	      READ_ULEB (abbrev_tag, entryptr, unit_end);
	      if (tagno == -1)
		{
		  second_abbrev_tag = abbrev_tag;
		  tagno = 0;
		  entryptr = entry_pool + entry_offset;
		  continue;
		}
	      if (abbrev_tag == 0)
		break;
	      if (tagno >= 0)
		printf ("%s<%lu>",
		        (tagno == 0 && second_abbrev_tag == 0 ? " " : "\n\t"),
			(unsigned long) abbrev_tag);

	      for (entry = abbrev_lookup;
		   entry < abbrev_lookup + abbrev_lookup_used;
		   entry++)
		if (entry->abbrev_tag == abbrev_tag)
		  break;
	      if (entry >= abbrev_lookup + abbrev_lookup_used)
		{
		  warn (_("Undefined abbreviation tag %lu "
			  "in unit 0x%lx in the debug_names\n"),
			(long) abbrev_tag,
			(long) (unit_start - section->start));
		  break;
		}
	      abbrevptr = entry->abbrev_lookup_ptr;
	      READ_ULEB (dwarf_tag, abbrevptr, abbrev_table_end);
	      if (tagno >= 0)
		printf (" %s", get_TAG_name (dwarf_tag));
	      for (;;)
		{
		  dwarf_vma xindex, form;

		  READ_ULEB (xindex, abbrevptr, abbrev_table_end);
		  READ_ULEB (form, abbrevptr, abbrev_table_end);
		  if (xindex == 0 && form == 0)
		    break;

		  if (tagno >= 0)
		    printf (" %s", get_IDX_name (xindex));
		  entryptr = read_and_display_attr_value (0, form, 0,
							  unit_start, entryptr, unit_end,
							  0, 0, offset_size,
							  dwarf_version, NULL,
							  (tagno < 0), section,
							  NULL, '=', -1);
		}
	      ++tagno;
	    }
	  if (tagno <= 0)
	    printf (_(" <no entries>"));
	  putchar ('\n');
	}

      free (abbrev_lookup);
    }

  return 1;
}