void Binary::shift(size_t value) {
  Header& header = this->header();

  // Offset of the load commands table
  const uint64_t loadcommands_start = is64_ ? sizeof(details::mach_header_64) :
                                              sizeof(details::mach_header);

  // +------------------------+ <---------- __TEXT.start
  // |      Mach-O Header     |
  // +------------------------+ <===== loadcommands_start
  // |                        |
  // | Load Command Table     |
  // |                        |
  // +------------------------+ <===== loadcommands_end
  // |************************|
  // |************************| Assembly code
  // |************************|
  // +------------------------+ <---------- __TEXT.end
  const uint64_t loadcommands_end = loadcommands_start + header.sizeof_cmds();

  // Segment that wraps this load command table
  SegmentCommand* load_cmd_segment = segment_from_offset(loadcommands_end);
  if (load_cmd_segment == nullptr) {
    LIEF_WARN("Can't find segment associated with last load command");
    return;
  }
  LIEF_DEBUG("LC Table wrapped by {} / End offset: 0x{:x} (size: {:x})",
             load_cmd_segment->name(), loadcommands_end, load_cmd_segment->data_.size());
  load_cmd_segment->content_insert(loadcommands_end, value);

  // 1. Shift all commands
  // =====================
  for (std::unique_ptr<LoadCommand>& cmd : commands_) {
    if (cmd->command_offset() >= loadcommands_end) {
      cmd->command_offset(cmd->command_offset() + value);
    }
  }

  shift_command(value, loadcommands_end);

  // Shift Segment and sections
  // ==========================
  for (SegmentCommand* segment : segments_) {
    // Extend the virtual size of the segment containing our shift
    if (segment->file_offset() <= loadcommands_end &&
        loadcommands_end < (segment->file_offset() + segment->file_size()))
    {
      LIEF_DEBUG("Extending '{}' by {:x}", segment->name(), value);
      segment->virtual_size(segment->virtual_size() + value);
      segment->file_size(segment->file_size() + value);

      for (const std::unique_ptr<Section>& section : segment->sections_) {
        if (section->offset() >= loadcommands_end) {
          section->offset(section->offset() + value);
          section->virtual_address(section->virtual_address() + value);
        }
      }
    } else {
      if (segment->file_offset() >= loadcommands_end) {
        segment->file_offset(segment->file_offset() + value);
        segment->virtual_address(segment->virtual_address() + value);
      }

      for (const std::unique_ptr<Section>& section : segment->sections_) {
        if (section->offset() >= loadcommands_end) {
          section->offset(section->offset() + value);
          section->virtual_address(section->virtual_address() + value);
        }

        if (section->type() == MACHO_SECTION_TYPES::S_ZEROFILL) {
          section->virtual_address(section->virtual_address() + value);
        }
      }
    }
  }
  refresh_seg_offset();
}