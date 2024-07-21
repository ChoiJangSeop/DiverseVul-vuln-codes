void sendCertificateVerify(SSL& ssl, BufferOutput buffer)
{
    if (ssl.GetError()) return;

    if(ssl.getCrypto().get_certManager().sendBlankCert()) return;

    CertificateVerify  verify;
    verify.Build(ssl);
    RecordLayerHeader  rlHeader;
    HandShakeHeader    hsHeader;
    mySTL::auto_ptr<output_buffer> out(NEW_YS output_buffer);

    buildHeaders(ssl, hsHeader, rlHeader, verify);
    buildOutput(*out.get(), rlHeader, hsHeader, verify);
    hashHandShake(ssl, *out.get());

    if (buffer == buffered)
        ssl.addBuffer(out.release());
    else
        ssl.Send(out->get_buffer(), out->get_size());
}