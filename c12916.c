uint64_t ThreadCommand::pc() const {
  uint64_t entry = 0;
  switch(architecture_) {
    case CPU_TYPES::CPU_TYPE_X86:
      {
        entry = reinterpret_cast<const details::x86_thread_state_t*>(state_.data())->eip;
        break;
      }

    case CPU_TYPES::CPU_TYPE_X86_64:
      {
        entry = reinterpret_cast<const details::x86_thread_state64_t*>(state_.data())->rip;
        break;
      }

    case CPU_TYPES::CPU_TYPE_ARM:
      {
        entry = reinterpret_cast<const details::arm_thread_state_t*>(state_.data())->r15;
        break;
      }

    case CPU_TYPES::CPU_TYPE_ARM64:
      {
        entry = reinterpret_cast<const details::arm_thread_state64_t*>(state_.data())->pc;
        break;
      }
    default:
      {
        LIEF_ERR("Unknown architecture");
      }
  }
  return entry;
}