boost::optional<SaplingNotePlaintext> SaplingNotePlaintext::decrypt(
    const SaplingEncCiphertext &ciphertext,
    const uint256 &epk,
    const uint256 &esk,
    const uint256 &pk_d,
    const uint256 &cmu
)
{
    auto pt = AttemptSaplingEncDecryption(ciphertext, epk, esk, pk_d);
    if (!pt) {
        return boost::none;
    }

    // Deserialize from the plaintext
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << pt.get();

    SaplingNotePlaintext ret;
    ss >> ret;

    uint256 cmu_expected;
    if (!librustzcash_sapling_compute_cm(
        ret.d.data(),
        pk_d.begin(),
        ret.value(),
        ret.rcm.begin(),
        cmu_expected.begin()
    ))
    {
        return boost::none;
    }

    if (cmu_expected != cmu) {
        return boost::none;
    }

    assert(ss.size() == 0);

    return ret;
}