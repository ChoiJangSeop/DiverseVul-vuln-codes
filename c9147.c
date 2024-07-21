TEST_F(OwnedImplTest, ReserveReuse) {
  Buffer::OwnedImpl buffer;
  static constexpr uint64_t NumIovecs = 2;
  Buffer::RawSlice iovecs[NumIovecs];

  // Reserve some space and leave it uncommitted.
  uint64_t num_reserved = buffer.reserve(8192, iovecs, NumIovecs);
  EXPECT_EQ(1, num_reserved);
  const void* first_slice = iovecs[0].mem_;

  // Reserve more space and verify that it begins with the same slice from the last reservation.
  num_reserved = buffer.reserve(16384, iovecs, NumIovecs);
  EXPECT_EQ(2, num_reserved);
  EXPECT_EQ(first_slice, iovecs[0].mem_);
  const void* second_slice = iovecs[1].mem_;

  // Repeat the last reservation and verify that it yields the same slices.
  num_reserved = buffer.reserve(16384, iovecs, NumIovecs);
  EXPECT_EQ(2, num_reserved);
  EXPECT_EQ(first_slice, iovecs[0].mem_);
  EXPECT_EQ(second_slice, iovecs[1].mem_);
  expectSlices({{0, 12248, 12248}, {0, 8152, 8152}}, buffer);

  // Request a larger reservation, verify that the second entry is replaced with a block with a
  // larger size.
  num_reserved = buffer.reserve(30000, iovecs, NumIovecs);
  const void* third_slice = iovecs[1].mem_;
  EXPECT_EQ(2, num_reserved);
  EXPECT_EQ(first_slice, iovecs[0].mem_);
  EXPECT_EQ(12248, iovecs[0].len_);
  EXPECT_NE(second_slice, iovecs[1].mem_);
  EXPECT_EQ(30000 - iovecs[0].len_, iovecs[1].len_);
  expectSlices({{0, 12248, 12248}, {0, 8152, 8152}, {0, 20440, 20440}}, buffer);

  // Repeating a the reservation request for a smaller block returns the previous entry.
  num_reserved = buffer.reserve(16384, iovecs, NumIovecs);
  EXPECT_EQ(2, num_reserved);
  EXPECT_EQ(first_slice, iovecs[0].mem_);
  EXPECT_EQ(second_slice, iovecs[1].mem_);
  expectSlices({{0, 12248, 12248}, {0, 8152, 8152}, {0, 20440, 20440}}, buffer);

  // Repeat the larger reservation notice that it doesn't match the prior reservation for 30000
  // bytes.
  num_reserved = buffer.reserve(30000, iovecs, NumIovecs);
  EXPECT_EQ(2, num_reserved);
  EXPECT_EQ(first_slice, iovecs[0].mem_);
  EXPECT_EQ(12248, iovecs[0].len_);
  EXPECT_NE(second_slice, iovecs[1].mem_);
  EXPECT_NE(third_slice, iovecs[1].mem_);
  EXPECT_EQ(30000 - iovecs[0].len_, iovecs[1].len_);
  expectSlices({{0, 12248, 12248}, {0, 8152, 8152}, {0, 20440, 20440}, {0, 20440, 20440}}, buffer);

  // Commit the most recent reservation and verify the representation.
  buffer.commit(iovecs, num_reserved);
  expectSlices({{12248, 0, 12248}, {0, 8152, 8152}, {0, 20440, 20440}, {17752, 2688, 20440}},
               buffer);

  // Do another reservation.
  num_reserved = buffer.reserve(16384, iovecs, NumIovecs);
  EXPECT_EQ(2, num_reserved);
  expectSlices({{12248, 0, 12248},
                {0, 8152, 8152},
                {0, 20440, 20440},
                {17752, 2688, 20440},
                {0, 16344, 16344}},
               buffer);

  // And commit.
  buffer.commit(iovecs, num_reserved);
  expectSlices({{12248, 0, 12248},
                {0, 8152, 8152},
                {0, 20440, 20440},
                {20440, 0, 20440},
                {13696, 2648, 16344}},
               buffer);
}