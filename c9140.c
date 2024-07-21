TEST_F(OwnedImplTest, ReserveZeroCommit) {
  BufferFragmentImpl frag("", 0, nullptr);
  Buffer::OwnedImpl buf;
  buf.addBufferFragment(frag);
  buf.prepend("bbbbb");
  buf.add("");
  constexpr uint32_t reserve_slices = 16;
  Buffer::RawSlice slices[reserve_slices];
  const uint32_t allocated_slices = buf.reserve(1280, slices, reserve_slices);
  for (uint32_t i = 0; i < allocated_slices; ++i) {
    slices[i].len_ = 0;
  }
  buf.commit(slices, allocated_slices);
  os_fd_t pipe_fds[2] = {0, 0};
  auto& os_sys_calls = Api::OsSysCallsSingleton::get();
#ifdef WIN32
  ASSERT_EQ(os_sys_calls.socketpair(AF_INET, SOCK_STREAM, 0, pipe_fds).rc_, 0);
#else
  ASSERT_EQ(pipe(pipe_fds), 0);
#endif
  Network::IoSocketHandleImpl io_handle(pipe_fds[0]);
  ASSERT_EQ(os_sys_calls.setsocketblocking(pipe_fds[0], false).rc_, 0);
  ASSERT_EQ(os_sys_calls.setsocketblocking(pipe_fds[1], false).rc_, 0);
  const uint32_t max_length = 1953;
  std::string data(max_length, 'e');
  const ssize_t rc = os_sys_calls.write(pipe_fds[1], data.data(), max_length).rc_;
  ASSERT_GT(rc, 0);
  const uint32_t previous_length = buf.length();
  Api::IoCallUint64Result result = buf.read(io_handle, max_length);
  ASSERT_EQ(result.rc_, static_cast<uint64_t>(rc));
  ASSERT_EQ(os_sys_calls.close(pipe_fds[1]).rc_, 0);
  ASSERT_EQ(previous_length, buf.search(data.data(), rc, previous_length));
  EXPECT_EQ("bbbbb", buf.toString().substr(0, 5));
  expectSlices({{5, 0, 4056}, {1953, 2103, 4056}}, buf);
}