LoadCommand* Binary::add(const LoadCommand& command) {
  static constexpr uint32_t shift_value = 0x4000;
  const int32_t size_aligned = align(command.size(), pointer_size());

  // Check there is enough spaces between the load command table
  // and the raw content
  if (available_command_space_ < size_aligned) {
    shift(shift_value);
    available_command_space_ += shift_value;
    return add(command);
  }

  available_command_space_ -= size_aligned;

  Header& header = this->header();

  // Get border of the load command table
  const uint64_t loadcommands_start = is64_ ? sizeof(details::mach_header_64) :
                                              sizeof(details::mach_header);
  const uint64_t loadcommands_end = loadcommands_start + header.sizeof_cmds();

  // Update the Header according to the command that will be added
  header.sizeof_cmds(header.sizeof_cmds() + size_aligned);
  header.nb_cmds(header.nb_cmds() + 1);

  // Get the segment handling the LC table
  SegmentCommand* load_cmd_segment = segment_from_offset(loadcommands_end);
  if (load_cmd_segment == nullptr) {
    LIEF_WARN("Can't get the last load command");
    return nullptr;
  }

  span<const uint8_t> content_ref = load_cmd_segment->content();
  std::vector<uint8_t> content = {std::begin(content_ref), std::end(content_ref)};

  // Copy the command data
  std::copy(std::begin(command.data()), std::end(command.data()),
            std::begin(content) + loadcommands_end);

  load_cmd_segment->content(std::move(content));

  // Add the command in the Binary
  std::unique_ptr<LoadCommand> copy(command.clone());
  copy->command_offset(loadcommands_end);


  // Update cache
  if (DylibCommand::classof(copy.get())) {
    libraries_.push_back(copy->as<DylibCommand>());
  }

  if (SegmentCommand::classof(copy.get())) {
    add_cached_segment(*copy->as<SegmentCommand>());
  }
  LoadCommand* ptr = copy.get();
  commands_.push_back(std::move(copy));
  return ptr;
}