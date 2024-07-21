TEST_F(EncryptedRecordTest, TestAllPaddingHandshake) {
  addToQueue("17030100050123456789");
  EXPECT_CALL(*readAead_, _decrypt(_, _, 0))
      .WillOnce(Invoke([](std::unique_ptr<IOBuf>& buf, const IOBuf*, uint64_t) {
        expectSame(buf, "0123456789");
        return getBuf("16000000");
      }));
  EXPECT_NO_THROW(read_.read(queue_));
}