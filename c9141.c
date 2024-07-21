TEST_F(OwnedImplTest, ReadReserveAndCommit) {
  BufferFragmentImpl frag("", 0, nullptr);
  Buffer::OwnedImpl buf;
  buf.add("bbbbb");

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

  const uint32_t read_length = 32768;
  std::string data = "e";
  const ssize_t rc = os_sys_calls.write(pipe_fds[1], data.data(), data.size()).rc_;
  ASSERT_GT(rc, 0);
  Api::IoCallUint64Result result = buf.read(io_handle, read_length);
  ASSERT_EQ(result.rc_, static_cast<uint64_t>(rc));
  ASSERT_EQ(os_sys_calls.close(pipe_fds[1]).rc_, 0);
  EXPECT_EQ("bbbbbe", buf.toString());
  expectSlices({{6, 4050, 4056}}, buf);
}