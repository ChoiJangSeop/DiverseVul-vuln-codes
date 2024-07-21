static int DecodeBasicOcspResponse(byte* source, word32* ioIndex,
            OcspResponse* resp, word32 size, void* cm, void* heap, int noVerify)
{
    int    length;
    word32 idx = *ioIndex;
    word32 end_index;
    int    ret;
    int    sigLength;

    WOLFSSL_ENTER("DecodeBasicOcspResponse");
    (void)heap;

    if (GetSequence(source, &idx, &length, size) < 0)
        return ASN_PARSE_E;

    if (idx + length > size)
        return ASN_INPUT_E;
    end_index = idx + length;

    if ((ret = DecodeResponseData(source, &idx, resp, size)) < 0)
        return ret; /* ASN_PARSE_E, ASN_BEFORE_DATE_E, ASN_AFTER_DATE_E */

    /* Get the signature algorithm */
    if (GetAlgoId(source, &idx, &resp->sigOID, oidSigType, size) < 0)
        return ASN_PARSE_E;

    ret = CheckBitString(source, &idx, &sigLength, size, 1, NULL);
    if (ret != 0)
        return ret;

    resp->sigSz = sigLength;
    resp->sig = source + idx;
    idx += sigLength;

    /*
     * Check the length of the BasicOcspResponse against the current index to
     * see if there are certificates, they are optional.
     */
#ifndef WOLFSSL_NO_OCSP_OPTIONAL_CERTS
    if (idx < end_index)
    {
        DecodedCert cert;

        if (DecodeCerts(source, &idx, resp, size) < 0)
            return ASN_PARSE_E;

        InitDecodedCert(&cert, resp->cert, resp->certSz, heap);

        /* Don't verify if we don't have access to Cert Manager. */
        ret = ParseCertRelative(&cert, CERT_TYPE,
                                noVerify ? NO_VERIFY : VERIFY_OCSP, cm);
        if (ret < 0) {
            WOLFSSL_MSG("\tOCSP Responder certificate parsing failed");
            FreeDecodedCert(&cert);
            return ret;
        }

#ifndef WOLFSSL_NO_OCSP_ISSUER_CHECK
        if ((cert.extExtKeyUsage & EXTKEYUSE_OCSP_SIGN) == 0) {
            if (XMEMCMP(cert.subjectHash,
                        resp->single->issuerHash, OCSP_DIGEST_SIZE) == 0) {
                WOLFSSL_MSG("\tOCSP Response signed by issuer");
            }
            else {
                WOLFSSL_MSG("\tOCSP Responder key usage check failed");
    #ifdef OPENSSL_EXTRA
                resp->verifyError = OCSP_BAD_ISSUER;
    #else
                FreeDecodedCert(&cert);
                return BAD_OCSP_RESPONDER;
    #endif
            }
        }
#endif

        /* ConfirmSignature is blocking here */
        ret = ConfirmSignature(&cert.sigCtx,
            resp->response, resp->responseSz,
            cert.publicKey, cert.pubKeySize, cert.keyOID,
            resp->sig, resp->sigSz, resp->sigOID, NULL);

        FreeDecodedCert(&cert);

        if (ret != 0) {
            WOLFSSL_MSG("\tOCSP Confirm signature failed");
            return ASN_OCSP_CONFIRM_E;
        }
    }
    else
#endif /* WOLFSSL_NO_OCSP_OPTIONAL_CERTS */
    {
        Signer* ca;
        int sigValid = -1;

        #ifndef NO_SKID
            ca = GetCA(cm, resp->single->issuerKeyHash);
        #else
            ca = GetCA(cm, resp->single->issuerHash);
        #endif

        if (ca) {
            SignatureCtx sigCtx;
            InitSignatureCtx(&sigCtx, heap, INVALID_DEVID);

            /* ConfirmSignature is blocking here */
            sigValid = ConfirmSignature(&sigCtx, resp->response,
                resp->responseSz, ca->publicKey, ca->pubKeySize, ca->keyOID,
                                resp->sig, resp->sigSz, resp->sigOID, NULL);
        }
        if (ca == NULL || sigValid != 0) {
            WOLFSSL_MSG("\tOCSP Confirm signature failed");
            return ASN_OCSP_CONFIRM_E;
        }

        (void)noVerify;
    }

    *ioIndex = idx;
    return 0;
}