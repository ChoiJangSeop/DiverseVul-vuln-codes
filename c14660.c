LoadCommand* Binary::add(const SegmentCommand& segment) {
  /*
   * To add a new segment in a Mach-O file, we need to:
   *
   * 1. Allocate space for a new Load command: LC_SEGMENT_64 / LC_SEGMENT
   *    which must include the sections
   * 2. Allocate space for the content of the provided segment
   *
   * For #1, the logic is to shift all the content after the end of the load command table.
   * This modification is described in doc/sphinx/tutorials/11_macho_modification.rst.
   *
   * For #2, the easiest way is to place the content at the end of the Mach-O file and
   * to make the LC_SEGMENT point to this area. It works as expected as long as
   * the binary does not need to be signed.
   *
   * If the binary has to be signed, codesign and the underlying Apple libraries
   * enforce that there is not data after the __LINKEDIT segment, otherwise we get
   * this kind of error: "main executable failed strict validation".
   * To comply with this check, we can shift the __LINKEDIT segment (c.f. ``shift_linkedit(...)``)
   * such as the data of the new segment are located before __LINKEDIT.
   * Nevertheless, we can't shift __LINKEDIT by an arbitrary value. For ARM and ARM64,
   * ld/dyld enforces a segment alignment of "4 * 4096" as coded in ``Options::reconfigureDefaults``
   * of ``ld64-609/src/ld/Option.cpp``:
   *
   * ```cpp
   * ...
   * <rdar://problem/13070042> Only third party apps should have 16KB page segments by default
   * if (fEncryptable) {
   *  if (fSegmentAlignment == 4096)
   *    fSegmentAlignment = 4096*4;
   * }
   *
   * // <rdar://problem/12258065> ARM64 needs 16KB page size for user land code
   * // <rdar://problem/15974532> make armv7[s] use 16KB pages in user land code for iOS 8 or later
   * if (fArchitecture == CPU_TYPE_ARM64 || (fArchitecture == CPU_TYPE_ARM) ) {
   *   fSegmentAlignment = 4096*4;
   * }
   * ```
   * Therefore, we must shift __LINKEDIT by at least 4 * 0x1000 for Mach-O files targeting ARM
   */

  LIEF_DEBUG("Adding the new segment '{}' ({} bytes)", segment.name(), segment.content().size());
  const uint32_t alignment = page_size();
  const uint64_t new_fsize = align(segment.content().size(), alignment);
  SegmentCommand new_segment = segment;

  if (new_segment.file_size() == 0) {
    new_segment.file_size(new_fsize);
    new_segment.content_resize(new_fsize);
  }

  if (new_segment.virtual_size() == 0) {
    const uint64_t new_size = align(new_segment.file_size(), alignment);
    new_segment.virtual_size(new_size);
  }

  if (segment.sections().size() > 0) {
    new_segment.nb_sections_ = segment.sections().size();
  }

  if (is64_) {
    new_segment.command(LOAD_COMMAND_TYPES::LC_SEGMENT_64);
    size_t needed_size = sizeof(details::segment_command_64);
    needed_size += new_segment.numberof_sections() * sizeof(details::section_64);
    new_segment.size(needed_size);
  } else {
    new_segment.command(LOAD_COMMAND_TYPES::LC_SEGMENT);
    size_t needed_size = sizeof(details::segment_command_32);
    needed_size += new_segment.numberof_sections() * sizeof(details::section_32);
    new_segment.size(needed_size);
  }

  LIEF_DEBUG(" -> sizeof(LC_SEGMENT): {}", new_segment.size());

  // Insert the segment before __LINKEDIT
  const auto it_linkedit = std::find_if(std::begin(commands_), std::end(commands_),
      [] (const std::unique_ptr<LoadCommand>& cmd) {
        if (!SegmentCommand::classof(cmd.get())) {
          return false;
        }
        return cmd->as<SegmentCommand>()->name() == "__LINKEDIT";
      });

  const bool has_linkedit = it_linkedit != std::end(commands_);


  size_t pos = std::distance(std::begin(commands_), it_linkedit);

  LIEF_DEBUG(" -> index: {}", pos);

  auto* segment_added = add(new_segment, pos)->as<SegmentCommand>();

  if (segment_added == nullptr) {
    LIEF_WARN("Fail to insert new '{}' segment", segment.name());
    return nullptr;
  }

  if (!has_linkedit) {
    /* If there are not __LINKEDIT segment we can point the Segment's content to the EOF
     * NOTE(romain): I don't know if a binary without a __LINKEDIT segment exists
     */
    range_t new_va_ranges  = this->va_ranges();
    range_t new_off_ranges = off_ranges();
    if (segment.virtual_address() == 0 && segment_added->virtual_size() != 0) {
      const uint64_t new_va = align(new_va_ranges.end, alignment);
      segment_added->virtual_address(new_va);
      size_t current_va = segment_added->virtual_address();
      for (Section& section : segment_added->sections()) {
        section.virtual_address(current_va);
        current_va += section.size();
      }
    }

    if (segment.file_offset() == 0 && segment_added->virtual_size() != 0) {
      const uint64_t new_offset = align(new_off_ranges.end, alignment);
      segment_added->file_offset(new_offset);
      size_t current_offset = new_offset;
      for (Section& section : segment_added->sections()) {
        section.offset(current_offset);
        current_offset += section.size();
      }
    }
    refresh_seg_offset();
    return segment_added;
  }

  uint64_t lnk_offset = 0;
  uint64_t lnk_va     = 0;

  if (const SegmentCommand* lnk = get_segment("__LINKEDIT")) {
    lnk_offset = lnk->file_offset();
    lnk_va     = lnk->virtual_address();
  }

  // Make space for the content of the new segment
  shift_linkedit(new_fsize);
  LIEF_DEBUG(" -> offset         : 0x{:06x}", lnk_offset);
  LIEF_DEBUG(" -> virtual address: 0x{:06x}", lnk_va);

  segment_added->virtual_address(lnk_va);
  segment_added->virtual_size(segment_added->virtual_size());
  size_t current_va = segment_added->virtual_address();
  for (Section& section : segment_added->sections()) {
    section.virtual_address(current_va);
    current_va += section.size();
  }

  segment_added->file_offset(lnk_offset);
  size_t current_offset = lnk_offset;
  for (Section& section : segment_added->sections()) {
    section.offset(current_offset);
    current_offset += section.size();
  }

  refresh_seg_offset();
  return segment_added;
}