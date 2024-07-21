TEST_F(OwnedImplTest, ReserveCommit) {
  // This fragment will later be added to the buffer. It is declared in an enclosing scope to
  // ensure it is not destructed until after the buffer is.
  const std::string input = "Hello, world";
  BufferFragmentImpl fragment(input.c_str(), input.size(), nullptr);

  {
    Buffer::OwnedImpl buffer;
    // A zero-byte reservation should fail.
    static constexpr uint64_t NumIovecs = 16;
    Buffer::RawSlice iovecs[NumIovecs];
    uint64_t num_reserved = buffer.reserve(0, iovecs, NumIovecs);
    EXPECT_EQ(0, num_reserved);
    clearReservation(iovecs, num_reserved, buffer);
    EXPECT_EQ(0, buffer.length());

    // Test and commit a small reservation. This should succeed.
    num_reserved = buffer.reserve(1, iovecs, NumIovecs);
    EXPECT_EQ(1, num_reserved);
    // The implementation might provide a bigger reservation than requested.
    EXPECT_LE(1, iovecs[0].len_);
    iovecs[0].len_ = 1;
    commitReservation(iovecs, num_reserved, buffer);
    EXPECT_EQ(1, buffer.length());

    // Request a reservation that fits in the remaining space at the end of the last slice.
    num_reserved = buffer.reserve(1, iovecs, NumIovecs);
    EXPECT_EQ(1, num_reserved);
    EXPECT_LE(1, iovecs[0].len_);
    iovecs[0].len_ = 1;
    const void* slice1 = iovecs[0].mem_;
    clearReservation(iovecs, num_reserved, buffer);

    // Request a reservation that is too large to fit in the remaining space at the end of
    // the last slice, and allow the buffer to use only one slice. This should result in the
    // creation of a new slice within the buffer.
    num_reserved = buffer.reserve(4096 - sizeof(OwnedSlice), iovecs, 1);
    EXPECT_EQ(1, num_reserved);
    EXPECT_NE(slice1, iovecs[0].mem_);
    clearReservation(iovecs, num_reserved, buffer);

    // Request the same size reservation, but allow the buffer to use multiple slices. This
    // should result in the buffer creating a second slice and splitting the reservation between the
    // last two slices.
    num_reserved = buffer.reserve(4096 - sizeof(OwnedSlice), iovecs, NumIovecs);
    EXPECT_EQ(2, num_reserved);
    EXPECT_EQ(slice1, iovecs[0].mem_);
    clearReservation(iovecs, num_reserved, buffer);

    // Request a reservation that too big to fit in the existing slices. This should result
    // in the creation of a third slice.
    expectSlices({{1, 4055, 4056}}, buffer);
    buffer.reserve(4096 - sizeof(OwnedSlice), iovecs, NumIovecs);
    expectSlices({{1, 4055, 4056}, {0, 4056, 4056}}, buffer);
    const void* slice2 = iovecs[1].mem_;
    num_reserved = buffer.reserve(8192, iovecs, NumIovecs);
    expectSlices({{1, 4055, 4056}, {0, 4056, 4056}, {0, 4056, 4056}}, buffer);
    EXPECT_EQ(3, num_reserved);
    EXPECT_EQ(slice1, iovecs[0].mem_);
    EXPECT_EQ(slice2, iovecs[1].mem_);
    clearReservation(iovecs, num_reserved, buffer);

    // Append a fragment to the buffer, and then request a small reservation. The buffer
    // should make a new slice to satisfy the reservation; it cannot safely use any of
    // the previously seen slices, because they are no longer at the end of the buffer.
    expectSlices({{1, 4055, 4056}}, buffer);
    buffer.addBufferFragment(fragment);
    EXPECT_EQ(13, buffer.length());
    num_reserved = buffer.reserve(1, iovecs, NumIovecs);
    expectSlices({{1, 4055, 4056}, {12, 0, 12}, {0, 4056, 4056}}, buffer);
    EXPECT_EQ(1, num_reserved);
    EXPECT_NE(slice1, iovecs[0].mem_);
    commitReservation(iovecs, num_reserved, buffer);
    EXPECT_EQ(14, buffer.length());
  }
}