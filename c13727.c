read_and_display_attr_value (unsigned long           attribute,
			     unsigned long           form,
			     dwarf_signed_vma        implicit_const,
			     unsigned char *         start,
			     unsigned char *         data,
			     unsigned char *         end,
			     dwarf_vma               cu_offset,
			     dwarf_vma               pointer_size,
			     dwarf_vma               offset_size,
			     int                     dwarf_version,
			     debug_info *            debug_info_p,
			     int                     do_loc,
			     struct dwarf_section *  section,
			     struct cu_tu_set *      this_set,
			     char                    delimiter,
			     int                     level)
{
  dwarf_signed_vma svalue;
  dwarf_vma uvalue = 0;
  dwarf_vma uvalue_hi = 0;
  unsigned char *block_start = NULL;
  unsigned char *orig_data = data;

  if (data > end || (data == end && form != DW_FORM_flag_present))
    {
      warn (_("Corrupt attribute\n"));
      return data;
    }

  if (do_wide && ! do_loc)
    {
      /* PR 26847: Display the name of the form.  */
      const char * name = get_FORM_name (form);

      /* For convenience we skip the DW_FORM_ prefix to the name.  */
      if (name[0] == 'D')
	name += 8; /* strlen ("DW_FORM_")  */
      printf ("%c(%s)", delimiter, name);
    }

  switch (form)
    {
    case DW_FORM_ref_addr:
      if (dwarf_version == 2)
	SAFE_BYTE_GET_AND_INC (uvalue, data, pointer_size, end);
      else if (dwarf_version > 2)
	SAFE_BYTE_GET_AND_INC (uvalue, data, offset_size, end);
      else
	error (_("Internal error: DW_FORM_ref_addr is not supported in DWARF version 1.\n"));
      break;

    case DW_FORM_addr:
      SAFE_BYTE_GET_AND_INC (uvalue, data, pointer_size, end);
      break;

    case DW_FORM_strp_sup:
    case DW_FORM_strp:
    case DW_FORM_line_strp:
    case DW_FORM_sec_offset:
    case DW_FORM_GNU_ref_alt:
    case DW_FORM_GNU_strp_alt:
      SAFE_BYTE_GET_AND_INC (uvalue, data, offset_size, end);
      break;

    case DW_FORM_flag_present:
      uvalue = 1;
      break;

    case DW_FORM_ref1:
    case DW_FORM_flag:
    case DW_FORM_data1:
    case DW_FORM_strx1:
    case DW_FORM_addrx1:
      SAFE_BYTE_GET_AND_INC (uvalue, data, 1, end);
      break;

    case DW_FORM_ref2:
    case DW_FORM_data2:
    case DW_FORM_strx2:
    case DW_FORM_addrx2:
      SAFE_BYTE_GET_AND_INC (uvalue, data, 2, end);
      break;

    case DW_FORM_strx3:
    case DW_FORM_addrx3:
      SAFE_BYTE_GET_AND_INC (uvalue, data, 3, end);
      break;

    case DW_FORM_ref_sup4:
    case DW_FORM_ref4:
    case DW_FORM_data4:
    case DW_FORM_strx4:
    case DW_FORM_addrx4:
      SAFE_BYTE_GET_AND_INC (uvalue, data, 4, end);
      break;

    case DW_FORM_ref_sup8:
    case DW_FORM_ref8:
    case DW_FORM_data8:
    case DW_FORM_ref_sig8:
      SAFE_BYTE_GET_AND_INC (uvalue, data, 8, end);
      break;

    case DW_FORM_data16:
      SAFE_BYTE_GET_AND_INC (uvalue, data, 8, end);
      SAFE_BYTE_GET_AND_INC (uvalue_hi, data, 8, end);
      if (byte_get != byte_get_little_endian)
	{
	  dwarf_vma utmp = uvalue;
	  uvalue = uvalue_hi;
	  uvalue_hi = utmp;
	}
      break;

    case DW_FORM_sdata:
      READ_SLEB (svalue, data, end);
      uvalue = svalue;
      break;

    case DW_FORM_GNU_str_index:
    case DW_FORM_strx:
    case DW_FORM_ref_udata:
    case DW_FORM_udata:
    case DW_FORM_GNU_addr_index:
    case DW_FORM_addrx:
    case DW_FORM_loclistx:
    case DW_FORM_rnglistx:
      READ_ULEB (uvalue, data, end);
      break;

    case DW_FORM_indirect:
      READ_ULEB (form, data, end);
      if (!do_loc)
	printf ("%c%s", delimiter, get_FORM_name (form));
      if (form == DW_FORM_implicit_const)
	READ_SLEB (implicit_const, data, end);
      return read_and_display_attr_value (attribute, form, implicit_const,
					  start, data, end,
					  cu_offset, pointer_size,
					  offset_size, dwarf_version,
					  debug_info_p, do_loc,
					  section, this_set, delimiter, level);

    case DW_FORM_implicit_const:
      uvalue = implicit_const;
      break;

    default:
      break;
    }

  switch (form)
    {
    case DW_FORM_ref_addr:
      if (!do_loc)
	printf ("%c<0x%s>", delimiter, dwarf_vmatoa ("x", uvalue));
      break;

    case DW_FORM_GNU_ref_alt:
      if (!do_loc)
	{
	  if (do_wide)
	    /* We have already printed the form name.  */
	    printf ("%c<0x%s>", delimiter, dwarf_vmatoa ("x", uvalue));
	  else
	    printf ("%c<alt 0x%s>", delimiter, dwarf_vmatoa ("x", uvalue));
	}
      /* FIXME: Follow the reference...  */
      break;

    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref_sup4:
    case DW_FORM_ref_udata:
      if (!do_loc)
	printf ("%c<0x%s>", delimiter, dwarf_vmatoa ("x", uvalue + cu_offset));
      break;

    case DW_FORM_data4:
    case DW_FORM_addr:
    case DW_FORM_sec_offset:
      if (!do_loc)
	printf ("%c0x%s", delimiter, dwarf_vmatoa ("x", uvalue));
      break;

    case DW_FORM_flag_present:
    case DW_FORM_flag:
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_sdata:
      if (!do_loc)
	printf ("%c%s", delimiter, dwarf_vmatoa ("d", uvalue));
      break;

    case DW_FORM_udata:
      if (!do_loc)
	printf ("%c%s", delimiter, dwarf_vmatoa ("u", uvalue));
      break;

    case DW_FORM_implicit_const:
      if (!do_loc)
	printf ("%c%s", delimiter, dwarf_vmatoa ("d", implicit_const));
      break;

    case DW_FORM_ref_sup8:
    case DW_FORM_ref8:
    case DW_FORM_data8:
      if (!do_loc)
	{
	  dwarf_vma utmp = uvalue;
	  if (form == DW_FORM_ref8)
	    utmp += cu_offset;
	  printf ("%c0x%s", delimiter, dwarf_vmatoa ("x", utmp));
	}
      break;

    case DW_FORM_data16:
      if (!do_loc)
	printf (" 0x%s%s",
		uvalue_hi == 0 ? "" : dwarf_vmatoa ("x", uvalue_hi),
		dwarf_vmatoa_1 ("x", uvalue, uvalue_hi == 0 ? 0 : 8));
      break;

    case DW_FORM_string:
      if (!do_loc)
	printf ("%c%.*s", delimiter, (int) (end - data), data);
      data += strnlen ((char *) data, end - data);
      if (data < end)
	data++;
      break;

    case DW_FORM_block:
    case DW_FORM_exprloc:
      READ_ULEB (uvalue, data, end);
    do_block:
      block_start = data;
      if (block_start >= end)
	{
	  warn (_("Block ends prematurely\n"));
	  uvalue = 0;
	  block_start = end;
	}

      uvalue = check_uvalue (block_start, uvalue, end);

      data = block_start + uvalue;
      if (!do_loc)
	{
	  unsigned char op;

	  SAFE_BYTE_GET (op, block_start, sizeof (op), end);
	  if (op != DW_OP_addrx)
	    data = display_block (block_start, uvalue, end, delimiter);
	}
      break;

    case DW_FORM_block1:
      SAFE_BYTE_GET_AND_INC (uvalue, data, 1, end);
      goto do_block;

    case DW_FORM_block2:
      SAFE_BYTE_GET_AND_INC (uvalue, data, 2, end);
      goto do_block;

    case DW_FORM_block4:
      SAFE_BYTE_GET_AND_INC (uvalue, data, 4, end);
      goto do_block;

    case DW_FORM_strp:
      if (!do_loc)
	{
	  if (do_wide)
	    /* We have already displayed the form name.  */
	    printf (_("%c(offset: 0x%s): %s"), delimiter,
		    dwarf_vmatoa ("x", uvalue),
		    fetch_indirect_string (uvalue));
	  else
	    printf (_("%c(indirect string, offset: 0x%s): %s"), delimiter,
		    dwarf_vmatoa ("x", uvalue),
		    fetch_indirect_string (uvalue));
	}
      break;

    case DW_FORM_line_strp:
      if (!do_loc)
	{
	  if (do_wide)
	    /* We have already displayed the form name.  */
	    printf (_("%c(offset: 0x%s): %s"), delimiter,
		    dwarf_vmatoa ("x", uvalue),
		    fetch_indirect_line_string (uvalue));
	  else
	    printf (_("%c(indirect line string, offset: 0x%s): %s"), delimiter,
		    dwarf_vmatoa ("x", uvalue),
		    fetch_indirect_line_string (uvalue));
	}
      break;

    case DW_FORM_GNU_str_index:
    case DW_FORM_strx:
    case DW_FORM_strx1:
    case DW_FORM_strx2:
    case DW_FORM_strx3:
    case DW_FORM_strx4:
      if (!do_loc)
	{
	  const char *suffix = section ? strrchr (section->name, '.') : NULL;
	  bool dwo = suffix && strcmp (suffix, ".dwo") == 0;

	  if (do_wide)
	    /* We have already displayed the form name.  */
	    printf (_("%c(offset: 0x%s): %s"), delimiter,
		    dwarf_vmatoa ("x", uvalue),
		    fetch_indexed_string (uvalue, this_set, offset_size, dwo,
	                                  debug_info_p->str_offsets_base));
	  else
	    printf (_("%c(indexed string: 0x%s): %s"), delimiter,
		    dwarf_vmatoa ("x", uvalue),
		    fetch_indexed_string (uvalue, this_set, offset_size, dwo,
	                                  debug_info_p->str_offsets_base));
	}
      break;

    case DW_FORM_GNU_strp_alt:
      if (!do_loc)
	{
	  if (do_wide)
	    /* We have already displayed the form name.  */
	    printf (_("%c(offset: 0x%s) %s"), delimiter,
		    dwarf_vmatoa ("x", uvalue),
		    fetch_alt_indirect_string (uvalue));
	  else
	    printf (_("%c(alt indirect string, offset: 0x%s) %s"), delimiter,
		    dwarf_vmatoa ("x", uvalue),
		    fetch_alt_indirect_string (uvalue));
	}
      break;

    case DW_FORM_indirect:
      /* Handled above.  */
      break;

    case DW_FORM_ref_sig8:
      if (!do_loc)
	printf ("%c%s: 0x%s", delimiter, do_wide ? "" : "signature",
		dwarf_vmatoa ("x", uvalue));
      break;

    case DW_FORM_GNU_addr_index:
    case DW_FORM_addrx:
    case DW_FORM_addrx1:
    case DW_FORM_addrx2:
    case DW_FORM_addrx3:
    case DW_FORM_addrx4:
    case DW_FORM_loclistx:
    case DW_FORM_rnglistx:
      if (!do_loc)
	{
	  dwarf_vma base;
	  dwarf_vma offset;

	  if (debug_info_p == NULL)
	    base = 0;
	  else if (debug_info_p->addr_base == DEBUG_INFO_UNAVAILABLE)
	    base = 0;
	  else
	    base = debug_info_p->addr_base;

	  offset = base + uvalue * pointer_size;

	  if (do_wide)
	    /* We have already displayed the form name.  */
	    if (form == DW_FORM_loclistx)
	      printf (_("%c(index: 0x%s): %s"), delimiter,
	              dwarf_vmatoa ("x", uvalue),
	              dwarf_vmatoa ("x", debug_info_p->loc_offsets [uvalue]));
	    else
	      printf (_("%c(index: 0x%s): %s"), delimiter,
	              dwarf_vmatoa ("x", uvalue),
	              dwarf_vmatoa ("x", fetch_indexed_addr (offset, pointer_size)));
	  else
	    if (form == DW_FORM_loclistx)
	      printf (_("%c(addr_index: 0x%s): %s"), delimiter,
	              dwarf_vmatoa ("x", uvalue),
	              dwarf_vmatoa ("x", debug_info_p->loc_offsets [uvalue]));
	    else
	      printf (_("%c(addr_index: 0x%s): %s"), delimiter,
	              dwarf_vmatoa ("x", uvalue),
	              dwarf_vmatoa ("x", fetch_indexed_addr (offset, pointer_size)));
	}
      break;

    case DW_FORM_strp_sup:
      if (!do_loc)
	printf ("%c<0x%s>", delimiter, dwarf_vmatoa ("x", uvalue + cu_offset));
      break;
      
    default:
      warn (_("Unrecognized form: 0x%lx\n"), form);
      /* What to do?  Consume a byte maybe?  */
      ++data;
      break;
    }

  if ((do_loc || do_debug_loc || do_debug_ranges || do_debug_info)
      && num_debug_info_entries == 0
      && debug_info_p != NULL)
    {
      switch (attribute)
	{
	case DW_AT_loclists_base:
	  if (debug_info_p->loclists_base)
	    warn (_("CU @ 0x%s has multiple loclists_base values"),
		  dwarf_vmatoa ("x", debug_info_p->cu_offset));
	  debug_info_p->loclists_base = uvalue;
	  break;
	case DW_AT_rnglists_base:
	  if (debug_info_p->rnglists_base)
	    warn (_("CU @ 0x%s has multiple rnglists_base values"),
	          dwarf_vmatoa ("x", debug_info_p->cu_offset));
	  debug_info_p->rnglists_base = uvalue;
	  break;
	case DW_AT_str_offsets_base:
	  if (debug_info_p->str_offsets_base)
	    warn (_("CU @ 0x%s has multiple str_offsets_base values"),
		  dwarf_vmatoa ("x", debug_info_p->cu_offset));
	  debug_info_p->str_offsets_base = uvalue;
	  break;

	case DW_AT_frame_base:
	  have_frame_base = 1;
	  /* Fall through.  */
	case DW_AT_location:
	case DW_AT_GNU_locviews:
	case DW_AT_string_length:
	case DW_AT_return_addr:
	case DW_AT_data_member_location:
	case DW_AT_vtable_elem_location:
	case DW_AT_segment:
	case DW_AT_static_link:
	case DW_AT_use_location:
	case DW_AT_call_value:
	case DW_AT_GNU_call_site_value:
	case DW_AT_call_data_value:
	case DW_AT_GNU_call_site_data_value:
	case DW_AT_call_target:
	case DW_AT_GNU_call_site_target:
	case DW_AT_call_target_clobbered:
	case DW_AT_GNU_call_site_target_clobbered:
	  if ((dwarf_version < 4
	       && (form == DW_FORM_data4 || form == DW_FORM_data8))
	      || form == DW_FORM_sec_offset
	      || form == DW_FORM_loclistx)
	    {
	      /* Process location list.  */
	      unsigned int lmax = debug_info_p->max_loc_offsets;
	      unsigned int num = debug_info_p->num_loc_offsets;

	      if (lmax == 0 || num >= lmax)
		{
		  lmax += 1024;
		  debug_info_p->loc_offsets = (dwarf_vma *)
		    xcrealloc (debug_info_p->loc_offsets,
			       lmax, sizeof (*debug_info_p->loc_offsets));
		  debug_info_p->loc_views = (dwarf_vma *)
		    xcrealloc (debug_info_p->loc_views,
			       lmax, sizeof (*debug_info_p->loc_views));
		  debug_info_p->have_frame_base = (int *)
		    xcrealloc (debug_info_p->have_frame_base,
			       lmax, sizeof (*debug_info_p->have_frame_base));
		  debug_info_p->max_loc_offsets = lmax;
		}
	      if (form == DW_FORM_loclistx)
		uvalue = fetch_indexed_value (num, loclists, debug_info_p->loclists_base);
	      else if (this_set != NULL)
		uvalue += this_set->section_offsets [DW_SECT_LOC];

	      debug_info_p->have_frame_base [num] = have_frame_base;
	      if (attribute != DW_AT_GNU_locviews)
		{
		  uvalue += debug_info_p->loclists_base;

		  /* Corrupt DWARF info can produce more offsets than views.
		     See PR 23062 for an example.  */
		  if (debug_info_p->num_loc_offsets
		      > debug_info_p->num_loc_views)
		    warn (_("More location offset attributes than DW_AT_GNU_locview attributes\n"));
		  else
		    {
		      debug_info_p->loc_offsets [num] = uvalue;
		      debug_info_p->num_loc_offsets++;
		    }
		}
	      else
		{
		  assert (debug_info_p->num_loc_views <= num);
		  num = debug_info_p->num_loc_views;
		  if (num > debug_info_p->num_loc_offsets)
		    warn (_("More DW_AT_GNU_locview attributes than location offset attributes\n"));
		  else
		    {
		      debug_info_p->loc_views [num] = uvalue;
		      debug_info_p->num_loc_views++;
		    }
		}
	    }
	  break;

	case DW_AT_low_pc:
	  if (need_base_address)
	    debug_info_p->base_address = uvalue;
	  break;

	case DW_AT_GNU_addr_base:
	case DW_AT_addr_base:
	  debug_info_p->addr_base = uvalue;
	  break;

	case DW_AT_GNU_ranges_base:
	  debug_info_p->ranges_base = uvalue;
	  break;

	case DW_AT_ranges:
	  if ((dwarf_version < 4
	       && (form == DW_FORM_data4 || form == DW_FORM_data8))
	      || form == DW_FORM_sec_offset
	      || form == DW_FORM_rnglistx)
	    {
	      /* Process range list.  */
	      unsigned int lmax = debug_info_p->max_range_lists;
	      unsigned int num = debug_info_p->num_range_lists;

	      if (lmax == 0 || num >= lmax)
		{
		  lmax += 1024;
		  debug_info_p->range_lists = (dwarf_vma *)
		    xcrealloc (debug_info_p->range_lists,
			       lmax, sizeof (*debug_info_p->range_lists));
		  debug_info_p->max_range_lists = lmax;
		}

	      if (form == DW_FORM_rnglistx)
		uvalue = fetch_indexed_value (uvalue, rnglists, 0);

	      debug_info_p->range_lists [num] = uvalue;
	      debug_info_p->num_range_lists++;
	    }
	  break;

	case DW_AT_GNU_dwo_name:
	case DW_AT_dwo_name:
	  if (need_dwo_info)
	    switch (form)
	      {
	      case DW_FORM_strp:
		add_dwo_name ((const char *) fetch_indirect_string (uvalue), cu_offset);
		break;
	      case DW_FORM_GNU_strp_alt:
		add_dwo_name ((const char *) fetch_alt_indirect_string (uvalue), cu_offset);
		break;
	      case DW_FORM_GNU_str_index:
	      case DW_FORM_strx:
	      case DW_FORM_strx1:
	      case DW_FORM_strx2:
	      case DW_FORM_strx3:
	      case DW_FORM_strx4:
		add_dwo_name (fetch_indexed_string (uvalue, this_set, offset_size, false,
		                                    debug_info_p->str_offsets_base),
			      cu_offset);
		break;
	      case DW_FORM_string:
		add_dwo_name ((const char *) orig_data, cu_offset);
		break;
	      default:
		warn (_("Unsupported form (%s) for attribute %s\n"),
		      get_FORM_name (form), get_AT_name (attribute));
		break;
	      }
	  break;

	case DW_AT_comp_dir:
	  /* FIXME: Also extract a build-id in a CU/TU.  */
	  if (need_dwo_info)
	    switch (form)
	      {
	      case DW_FORM_strp:
		add_dwo_dir ((const char *) fetch_indirect_string (uvalue), cu_offset);
		break;
	      case DW_FORM_GNU_strp_alt:
		add_dwo_dir (fetch_alt_indirect_string (uvalue), cu_offset);
		break;
	      case DW_FORM_line_strp:
		add_dwo_dir ((const char *) fetch_indirect_line_string (uvalue), cu_offset);
		break;
	      case DW_FORM_GNU_str_index:
	      case DW_FORM_strx:
	      case DW_FORM_strx1:
	      case DW_FORM_strx2:
	      case DW_FORM_strx3:
	      case DW_FORM_strx4:
		add_dwo_dir (fetch_indexed_string (uvalue, this_set, offset_size, false,
		                                   debug_info_p->str_offsets_base),
			     cu_offset);
		break;
	      case DW_FORM_string:
		add_dwo_dir ((const char *) orig_data, cu_offset);
		break;
	      default:
		warn (_("Unsupported form (%s) for attribute %s\n"),
		      get_FORM_name (form), get_AT_name (attribute));
		break;
	      }
	  break;

	case DW_AT_GNU_dwo_id:
	  if (need_dwo_info)
	    switch (form)
	      {
	      case DW_FORM_data8:
		/* FIXME: Record the length of the ID as well ?  */
		add_dwo_id ((const char *) (data - 8), cu_offset);
		break;
	      default:
		warn (_("Unsupported form (%s) for attribute %s\n"),
		      get_FORM_name (form), get_AT_name (attribute));
		break;
	      }
	  break;

	default:
	  break;
	}
    }

  if (do_loc || attribute == 0)
    return data;

  /* For some attributes we can display further information.  */
  switch (attribute)
    {
    case DW_AT_type:
      if (level >= 0 && level < MAX_CU_NESTING
	  && uvalue < (size_t) (end - start))
	{
	  bool is_signed = false;
	  abbrev_entry *type_abbrev;
	  unsigned char *type_data;
	  abbrev_map *map;

	  type_abbrev = get_type_abbrev_from_form (form, uvalue,
						   cu_offset, end,
						   section, NULL,
						   &type_data, &map);
	  if (type_abbrev != NULL)
	    {
	      get_type_signedness (type_abbrev, section, type_data,
				   map ? section->start + map->end : end,
				   map ? map->start : cu_offset,
				   pointer_size, offset_size, dwarf_version,
				   & is_signed, 0);
	    }
	  level_type_signed[level] = is_signed;
	}
      break;

    case DW_AT_inline:
      printf ("\t");
      switch (uvalue)
	{
	case DW_INL_not_inlined:
	  printf (_("(not inlined)"));
	  break;
	case DW_INL_inlined:
	  printf (_("(inlined)"));
	  break;
	case DW_INL_declared_not_inlined:
	  printf (_("(declared as inline but ignored)"));
	  break;
	case DW_INL_declared_inlined:
	  printf (_("(declared as inline and inlined)"));
	  break;
	default:
	  printf (_("  (Unknown inline attribute value: %s)"),
		  dwarf_vmatoa ("x", uvalue));
	  break;
	}
      break;

    case DW_AT_language:
      printf ("\t");
      switch (uvalue)
	{
	  /* Ordered by the numeric value of these constants.  */
	case DW_LANG_C89:		printf ("(ANSI C)"); break;
	case DW_LANG_C:			printf ("(non-ANSI C)"); break;
	case DW_LANG_Ada83:		printf ("(Ada)"); break;
	case DW_LANG_C_plus_plus:	printf ("(C++)"); break;
	case DW_LANG_Cobol74:		printf ("(Cobol 74)"); break;
	case DW_LANG_Cobol85:		printf ("(Cobol 85)"); break;
	case DW_LANG_Fortran77:		printf ("(FORTRAN 77)"); break;
	case DW_LANG_Fortran90:		printf ("(Fortran 90)"); break;
	case DW_LANG_Pascal83:		printf ("(ANSI Pascal)"); break;
	case DW_LANG_Modula2:		printf ("(Modula 2)"); break;
	  /* DWARF 2.1 values.	*/
	case DW_LANG_Java:		printf ("(Java)"); break;
	case DW_LANG_C99:		printf ("(ANSI C99)"); break;
	case DW_LANG_Ada95:		printf ("(ADA 95)"); break;
	case DW_LANG_Fortran95:		printf ("(Fortran 95)"); break;
	  /* DWARF 3 values.  */
	case DW_LANG_PLI:		printf ("(PLI)"); break;
	case DW_LANG_ObjC:		printf ("(Objective C)"); break;
	case DW_LANG_ObjC_plus_plus:	printf ("(Objective C++)"); break;
	case DW_LANG_UPC:		printf ("(Unified Parallel C)"); break;
	case DW_LANG_D:			printf ("(D)"); break;
	  /* DWARF 4 values.  */
	case DW_LANG_Python:		printf ("(Python)"); break;
	  /* DWARF 5 values.  */
	case DW_LANG_OpenCL:		printf ("(OpenCL)"); break;
	case DW_LANG_Go:		printf ("(Go)"); break;
	case DW_LANG_Modula3:		printf ("(Modula 3)"); break;
	case DW_LANG_Haskell:		printf ("(Haskell)"); break;
	case DW_LANG_C_plus_plus_03:	printf ("(C++03)"); break;
	case DW_LANG_C_plus_plus_11:	printf ("(C++11)"); break;
	case DW_LANG_OCaml:		printf ("(OCaml)"); break;
	case DW_LANG_Rust:		printf ("(Rust)"); break;
	case DW_LANG_C11:		printf ("(C11)"); break;
	case DW_LANG_Swift:		printf ("(Swift)"); break;
	case DW_LANG_Julia:		printf ("(Julia)"); break;
	case DW_LANG_Dylan:		printf ("(Dylan)"); break;
	case DW_LANG_C_plus_plus_14:	printf ("(C++14)"); break;
	case DW_LANG_Fortran03:		printf ("(Fortran 03)"); break;
	case DW_LANG_Fortran08:		printf ("(Fortran 08)"); break;
	case DW_LANG_RenderScript:	printf ("(RenderScript)"); break;
	  /* MIPS extension.  */
	case DW_LANG_Mips_Assembler:	printf ("(MIPS assembler)"); break;
	  /* UPC extension.  */
	case DW_LANG_Upc:		printf ("(Unified Parallel C)"); break;
	default:
	  if (uvalue >= DW_LANG_lo_user && uvalue <= DW_LANG_hi_user)
	    printf (_("(implementation defined: %s)"),
		    dwarf_vmatoa ("x", uvalue));
	  else
	    printf (_("(Unknown: %s)"), dwarf_vmatoa ("x", uvalue));
	  break;
	}
      break;

    case DW_AT_encoding:
      printf ("\t");
      switch (uvalue)
	{
	case DW_ATE_void:		printf ("(void)"); break;
	case DW_ATE_address:		printf ("(machine address)"); break;
	case DW_ATE_boolean:		printf ("(boolean)"); break;
	case DW_ATE_complex_float:	printf ("(complex float)"); break;
	case DW_ATE_float:		printf ("(float)"); break;
	case DW_ATE_signed:		printf ("(signed)"); break;
	case DW_ATE_signed_char:	printf ("(signed char)"); break;
	case DW_ATE_unsigned:		printf ("(unsigned)"); break;
	case DW_ATE_unsigned_char:	printf ("(unsigned char)"); break;
	  /* DWARF 2.1 values:  */
	case DW_ATE_imaginary_float:	printf ("(imaginary float)"); break;
	case DW_ATE_decimal_float:	printf ("(decimal float)"); break;
	  /* DWARF 3 values:  */
	case DW_ATE_packed_decimal:	printf ("(packed_decimal)"); break;
	case DW_ATE_numeric_string:	printf ("(numeric_string)"); break;
	case DW_ATE_edited:		printf ("(edited)"); break;
	case DW_ATE_signed_fixed:	printf ("(signed_fixed)"); break;
	case DW_ATE_unsigned_fixed:	printf ("(unsigned_fixed)"); break;
	  /* DWARF 4 values:  */
	case DW_ATE_UTF:		printf ("(unicode string)"); break;
	  /* DWARF 5 values:  */
	case DW_ATE_UCS:		printf ("(UCS)"); break;
	case DW_ATE_ASCII:		printf ("(ASCII)"); break;

	  /* HP extensions:  */
	case DW_ATE_HP_float80:		printf ("(HP_float80)"); break;
	case DW_ATE_HP_complex_float80:	printf ("(HP_complex_float80)"); break;
	case DW_ATE_HP_float128:	printf ("(HP_float128)"); break;
	case DW_ATE_HP_complex_float128:printf ("(HP_complex_float128)"); break;
	case DW_ATE_HP_floathpintel:	printf ("(HP_floathpintel)"); break;
	case DW_ATE_HP_imaginary_float80:	printf ("(HP_imaginary_float80)"); break;
	case DW_ATE_HP_imaginary_float128:	printf ("(HP_imaginary_float128)"); break;

	default:
	  if (uvalue >= DW_ATE_lo_user
	      && uvalue <= DW_ATE_hi_user)
	    printf (_("(user defined type)"));
	  else
	    printf (_("(unknown type)"));
	  break;
	}
      break;

    case DW_AT_accessibility:
      printf ("\t");
      switch (uvalue)
	{
	case DW_ACCESS_public:		printf ("(public)"); break;
	case DW_ACCESS_protected:	printf ("(protected)"); break;
	case DW_ACCESS_private:		printf ("(private)"); break;
	default:
	  printf (_("(unknown accessibility)"));
	  break;
	}
      break;

    case DW_AT_visibility:
      printf ("\t");
      switch (uvalue)
	{
	case DW_VIS_local:		printf ("(local)"); break;
	case DW_VIS_exported:		printf ("(exported)"); break;
	case DW_VIS_qualified:		printf ("(qualified)"); break;
	default:			printf (_("(unknown visibility)")); break;
	}
      break;

    case DW_AT_endianity:
      printf ("\t");
      switch (uvalue)
	{
	case DW_END_default:		printf ("(default)"); break;
	case DW_END_big:		printf ("(big)"); break;
	case DW_END_little:		printf ("(little)"); break;
	default:
	  if (uvalue >= DW_END_lo_user && uvalue <= DW_END_hi_user)
	    printf (_("(user specified)"));
	  else
	    printf (_("(unknown endianity)"));
	  break;
	}
      break;

    case DW_AT_virtuality:
      printf ("\t");
      switch (uvalue)
	{
	case DW_VIRTUALITY_none:	printf ("(none)"); break;
	case DW_VIRTUALITY_virtual:	printf ("(virtual)"); break;
	case DW_VIRTUALITY_pure_virtual:printf ("(pure_virtual)"); break;
	default:			printf (_("(unknown virtuality)")); break;
	}
      break;

    case DW_AT_identifier_case:
      printf ("\t");
      switch (uvalue)
	{
	case DW_ID_case_sensitive:	printf ("(case_sensitive)"); break;
	case DW_ID_up_case:		printf ("(up_case)"); break;
	case DW_ID_down_case:		printf ("(down_case)"); break;
	case DW_ID_case_insensitive:	printf ("(case_insensitive)"); break;
	default:			printf (_("(unknown case)")); break;
	}
      break;

    case DW_AT_calling_convention:
      printf ("\t");
      switch (uvalue)
	{
	case DW_CC_normal:	printf ("(normal)"); break;
	case DW_CC_program:	printf ("(program)"); break;
	case DW_CC_nocall:	printf ("(nocall)"); break;
	case DW_CC_pass_by_reference: printf ("(pass by ref)"); break;
	case DW_CC_pass_by_value: printf ("(pass by value)"); break;
	case DW_CC_GNU_renesas_sh: printf ("(Rensas SH)"); break;
	case DW_CC_GNU_borland_fastcall_i386: printf ("(Borland fastcall i386)"); break;
	default:
	  if (uvalue >= DW_CC_lo_user
	      && uvalue <= DW_CC_hi_user)
	    printf (_("(user defined)"));
	  else
	    printf (_("(unknown convention)"));
	}
      break;

    case DW_AT_ordering:
      printf ("\t");
      switch (uvalue)
	{
	case 255:
	case -1: printf (_("(undefined)")); break;
	case 0:  printf ("(row major)"); break;
	case 1:  printf ("(column major)"); break;
	}
      break;

    case DW_AT_decimal_sign:
      printf ("\t");
      switch (uvalue)
	{
	case DW_DS_unsigned:            printf (_("(unsigned)")); break;
	case DW_DS_leading_overpunch:   printf (_("(leading overpunch)")); break;
	case DW_DS_trailing_overpunch:  printf (_("(trailing overpunch)")); break;
	case DW_DS_leading_separate:    printf (_("(leading separate)")); break;
	case DW_DS_trailing_separate:   printf (_("(trailing separate)")); break;
	default:                        printf (_("(unrecognised)")); break;
	}
      break;

    case DW_AT_defaulted:
      printf ("\t");
      switch (uvalue)
	{
	case DW_DEFAULTED_no:           printf (_("(no)")); break;
	case DW_DEFAULTED_in_class:     printf (_("(in class)")); break;
	case DW_DEFAULTED_out_of_class: printf (_("(out of class)")); break;
	default:                        printf (_("(unrecognised)")); break;
	}
      break;

    case DW_AT_discr_list:
      printf ("\t");
      display_discr_list (form, uvalue, data, level);
      break;

    case DW_AT_frame_base:
      have_frame_base = 1;
      /* Fall through.  */
    case DW_AT_location:
    case DW_AT_loclists_base:
    case DW_AT_rnglists_base:
    case DW_AT_str_offsets_base:
    case DW_AT_string_length:
    case DW_AT_return_addr:
    case DW_AT_data_member_location:
    case DW_AT_vtable_elem_location:
    case DW_AT_segment:
    case DW_AT_static_link:
    case DW_AT_use_location:
    case DW_AT_call_value:
    case DW_AT_GNU_call_site_value:
    case DW_AT_call_data_value:
    case DW_AT_GNU_call_site_data_value:
    case DW_AT_call_target:
    case DW_AT_GNU_call_site_target:
    case DW_AT_call_target_clobbered:
    case DW_AT_GNU_call_site_target_clobbered:
      if ((dwarf_version < 4
	   && (form == DW_FORM_data4 || form == DW_FORM_data8))
	  || form == DW_FORM_sec_offset
	  || form == DW_FORM_loclistx)
	{
	  if (attribute != DW_AT_rnglists_base
	      && attribute != DW_AT_str_offsets_base)
	    printf (_(" (location list)"));
	}
      /* Fall through.  */
    case DW_AT_allocated:
    case DW_AT_associated:
    case DW_AT_data_location:
    case DW_AT_stride:
    case DW_AT_upper_bound:
    case DW_AT_lower_bound:
      if (block_start)
	{
	  int need_frame_base;

	  printf ("\t(");
	  need_frame_base = decode_location_expression (block_start,
							pointer_size,
							offset_size,
							dwarf_version,
							uvalue,
							cu_offset, section);
	  printf (")");
	  if (need_frame_base && !have_frame_base)
	    printf (_(" [without DW_AT_frame_base]"));
	}
      break;

    case DW_AT_data_bit_offset:
    case DW_AT_byte_size:
    case DW_AT_bit_size:
    case DW_AT_string_length_byte_size:
    case DW_AT_string_length_bit_size:
    case DW_AT_bit_stride:
      if (form == DW_FORM_exprloc)
	{
	  printf ("\t(");
	  (void) decode_location_expression (block_start, pointer_size,
					     offset_size, dwarf_version,
					     uvalue, cu_offset, section);
	  printf (")");
	}
      break;

    case DW_AT_import:
      {
	unsigned long abbrev_number;
	abbrev_entry *entry;

	entry = get_type_abbrev_from_form (form, uvalue, cu_offset, end,
					   section, & abbrev_number, NULL, NULL);
	if (entry == NULL)
	  {
	    if (form != DW_FORM_GNU_ref_alt)
	      warn (_("Offset %s used as value for DW_AT_import attribute of DIE at offset 0x%lx is too big.\n"),
		    dwarf_vmatoa ("x", uvalue),
		    (unsigned long) (orig_data - section->start));
	  }
	else
	  {
	    printf (_("\t[Abbrev Number: %ld"), abbrev_number);
	    printf (" (%s)", get_TAG_name (entry->tag));
	    printf ("]");
	  }
      }
      break;

    default:
      break;
    }

  return data;
}