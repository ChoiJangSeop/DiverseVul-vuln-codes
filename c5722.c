TEST(SnappyMessageCompressor, Fidelity) {
    auto testMessage = buildMessage();
    checkFidelity(testMessage, stdx::make_unique<NoopMessageCompressor>());
}