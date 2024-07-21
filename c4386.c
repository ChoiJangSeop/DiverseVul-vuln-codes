word32 DSA_Signer::Sign(const byte* sha_digest, byte* sig,
                        RandomNumberGenerator& rng)
{
    const Integer& p = key_.GetModulus();
    const Integer& q = key_.GetSubGroupOrder();
    const Integer& g = key_.GetSubGroupGenerator();
    const Integer& x = key_.GetPrivatePart();

    Integer k(rng, 1, q - 1);

    r_ =  a_exp_b_mod_c(g, k, p);
    r_ %= q;

    Integer H(sha_digest, SHA::DIGEST_SIZE);  // sha Hash(m)

    Integer kInv = k.InverseMod(q);
    s_ = (kInv * (H + x*r_)) % q;

    if (!(!!r_ && !!s_))
        return -1;

    int rSz = r_.ByteCount();

    if (rSz == 19) {
        sig[0] = 0;
        sig++;
    }
    
    r_.Encode(sig,  rSz);

    int sSz = s_.ByteCount();

    if (sSz == 19) {
        sig[rSz] = 0;
        sig++;
    }

    s_.Encode(sig + rSz, sSz);

    return 40;
}