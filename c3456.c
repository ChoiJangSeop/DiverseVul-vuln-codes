void CertManager::setPeerX509(X509* x)
{
    if (x == 0) return;

    X509_NAME* issuer   = x->GetIssuer();
    X509_NAME* subject  = x->GetSubject();
    ASN1_STRING* before = x->GetBefore();
    ASN1_STRING* after  = x->GetAfter();

    peerX509_ = NEW_YS X509(issuer->GetName(), issuer->GetLength(),
        subject->GetName(), subject->GetLength(), (const char*) before->data,
        before->length, (const char*) after->data, after->length);
}