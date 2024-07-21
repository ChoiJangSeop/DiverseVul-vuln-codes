void *enc_untrusted_realloc(void *ptr, size_t size) {
  MessageWriter input;
  input.Push(reinterpret_cast<uint64_t>(ptr));
  input.Push(static_cast<uint64_t>(size));

  MessageReader output;
  asylo::primitives::PrimitiveStatus status =
      asylo::host_call::NonSystemCallDispatcher(
          asylo::host_call::kReallocHandler, &input, &output);
  CheckStatusAndParamCount(status, output, "enc_untrusted_realloc", 2);

  void *result = output.next<void *>();
  int klinux_errno = output.next<int>();

  // realloc only sets the errno (ENOMEM) when output pointer is null and a
  // non-zero |size| is provided.
  if (!result && size != 0) {
    errno = FromkLinuxErrorNumber(klinux_errno);
  }
  return result;
}