read_sections_map (Bit_Chain *dat, int64_t size_comp, int64_t size_uncomp,
                   int64_t correction)
{
  BITCODE_RC *data;
  r2007_section *sections = NULL, *last_section = NULL, *section = NULL;
  BITCODE_RC *ptr, *ptr_end;
  int i, j = 0;

  data = read_system_page (dat, size_comp, size_uncomp, correction);
  if (!data)
    {
      LOG_ERROR ("Failed to read system page")
      return NULL;
    }

  ptr = data;
  ptr_end = data + size_uncomp;

  LOG_TRACE ("\n=== System Section (Section Map) ===\n")

  while (ptr < ptr_end)
    {
      section = (r2007_section *)calloc (1, sizeof (r2007_section));
      if (!section)
        {
          LOG_ERROR ("Out of memory");
          free (data);
          sections_destroy (sections); // the root
          return NULL;
        }

      bfr_read (section, &ptr, 64);

      LOG_TRACE ("\nSection [%d]:\n", j)
      LOG_TRACE ("  data size:     %" PRIu64 "\n", section->data_size)
      LOG_TRACE ("  max size:      %" PRIu64 "\n", section->max_size)
      LOG_TRACE ("  encryption:    %" PRIu64 "\n", section->encrypted)
      LOG_HANDLE ("  hashcode:      %" PRIx64 "\n", section->hashcode)
      LOG_HANDLE ("  name length:   %" PRIu64 "\n", section->name_length)
      LOG_TRACE ("  unknown:       %" PRIu64 "\n", section->unknown)
      LOG_TRACE ("  encoding:      %" PRIu64 "\n", section->encoded)
      LOG_TRACE ("  num pages:     %" PRIu64 "\n", section->num_pages);

      // debugging sanity
#if 1
      /* compressed */
      if (section->data_size > 10 * dat->size
          || section->name_length >= (int64_t)dat->size)
        {
          LOG_ERROR ("Invalid System Section");
          free (section);
          free (data);
          sections_destroy (sections); // the root
          return NULL;
        }
        // assert(section->data_size < dat->size + 0x100000);
        // assert(section->max_size  < dat->size + 0x100000);
        // assert(section->num_pages < DBG_MAX_COUNT);
#endif
      // section->next = NULL;
      // section->pages = NULL;
      // section->name = NULL;

      if (!sections)
        {
          sections = last_section = section;
        }
      else
        {
          last_section->next = section;
          last_section = section;
        }

      j++;
      if (ptr >= ptr_end)
        break;

      // Section Name (wchar)
      section->name = bfr_read_string (&ptr, section->name_length);
#ifdef HAVE_NATIVE_WCHAR2
      LOG_TRACE ("  name:          " FORMAT_TU "\n\n",
                 (BITCODE_TU)section->name)
#else
      LOG_TRACE ("  name:          ")
      LOG_TEXT_UNICODE (TRACE, section->name)
      LOG_TRACE ("\n\n")
#endif
      section->type = dwg_section_wtype (section->name);

      if (section->num_pages <= 0)
        continue;
      if (section->num_pages > 0xf0000)
        {
          LOG_ERROR ("Invalid num_pages %lu, skip", (unsigned long)section->num_pages);
          continue;
        }

      section->pages = (r2007_section_page **)calloc (
          (size_t)section->num_pages, sizeof (r2007_section_page *));
      if (!section->pages)
        {
          LOG_ERROR ("Out of memory");
          free (data);
          if (sections)
            sections_destroy (sections); // the root
          else
            sections_destroy (section);
          return NULL;
        }

      for (i = 0; i < section->num_pages; i++)
        {
          section->pages[i]
              = (r2007_section_page *)calloc (1, sizeof (r2007_section_page));
          if (!section->pages[i])
            {
              LOG_ERROR ("Out of memory");
              free (data);
              if (sections)
                sections_destroy (sections); // the root
              else
                sections_destroy (section);
              return NULL;
            }

          if (ptr + 56 >= ptr_end)
            {
              LOG_ERROR ("Section[%d]->pages[%d] overflow", j, i);
              free (section->pages[i]);
              section->num_pages = i; // skip this last section
              break;
            }
          bfr_read (section->pages[i], &ptr, 56);

          LOG_TRACE (" Page[%d]: ", i)
          LOG_TRACE (" offset: 0x%07" PRIx64, section->pages[i]->offset);
          LOG_TRACE (" size: %5" PRIu64, section->pages[i]->size);
          LOG_TRACE (" id: %4" PRIu64, section->pages[i]->id);
          LOG_TRACE (" uncomp_size: %5" PRIu64 "\n",
                     section->pages[i]->uncomp_size);
          LOG_HANDLE (" comp_size: %5" PRIu64, section->pages[i]->comp_size);
          LOG_HANDLE (" checksum: %016" PRIx64, section->pages[i]->checksum);
          LOG_HANDLE (" crc64: %016" PRIx64 "\n", section->pages[i]->crc);
          // debugging sanity
          if (section->pages[i]->size >= DBG_MAX_SIZE
              || section->pages[i]->uncomp_size >= DBG_MAX_SIZE
              || section->pages[i]->comp_size >= DBG_MAX_SIZE)
            {
              LOG_ERROR ("Invalid section->pages[%d] size", i);
              free (data);
              free (section->pages[i]);
              section->num_pages = i; // skip this last section
              return sections;
            }
          assert (section->pages[i]->size < DBG_MAX_SIZE);
          assert (section->pages[i]->uncomp_size < DBG_MAX_SIZE);
          assert (section->pages[i]->comp_size < DBG_MAX_SIZE);
        }
    }

  free (data);
  return sections;
}