CertDecoder::CertDecoder(Source& s, bool decode, SignerList* signers,
                         bool noVerify, CertType ct)
    : BER_Decoder(s), certBegin_(0), sigIndex_(0), sigLength_(0),
      signature_(0), verify_(!noVerify)
{
    issuer_[0] = 0;
    subject_[0] = 0;

    if (decode)
        Decode(signers, ct);

}