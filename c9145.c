TEST_F(OwnedImplTest, AddEmptyFragment) {
  char input[] = "hello world";
  BufferFragmentImpl frag1(input, 11, [](const void*, size_t, const BufferFragmentImpl*) {});
  BufferFragmentImpl frag2("", 0, [this](const void*, size_t, const BufferFragmentImpl*) {
    release_callback_called_ = true;
  });
  Buffer::OwnedImpl buffer;
  buffer.addBufferFragment(frag1);
  EXPECT_EQ(11, buffer.length());

  buffer.addBufferFragment(frag2);
  EXPECT_EQ(11, buffer.length());

  buffer.drain(11);
  EXPECT_EQ(0, buffer.length());
  EXPECT_TRUE(release_callback_called_);
}