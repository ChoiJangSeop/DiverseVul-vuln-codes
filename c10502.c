CryptRsaDecrypt(
		TPM2B               *dOut,          // OUT: the decrypted data
		TPM2B               *cIn,           // IN: the data to decrypt
		OBJECT              *key,           // IN: the key to use for decryption
		TPMT_RSA_DECRYPT    *scheme,        // IN: the padding scheme
		const TPM2B         *label          // IN: in case it is needed for the scheme
		)
{
    TPM_RC                 retVal;
    // Make sure that the necessary parameters are provided
    pAssert(cIn != NULL && dOut != NULL && key != NULL);
    // Size is checked to make sure that the encrypted value is the right size
    if(cIn->size != key->publicArea.unique.rsa.t.size)
	ERROR_RETURN(TPM_RC_SIZE);
    TEST(scheme->scheme);
    // For others that do padding, do the decryption in place and then
    // go handle the decoding.
    retVal = RSADP(cIn, key);
    if(retVal == TPM_RC_SUCCESS)
	{
	    // Remove padding
	    switch(scheme->scheme)
		{
		  case ALG_NULL_VALUE:
		    if(dOut->size < cIn->size)
			return TPM_RC_VALUE;
		    MemoryCopy2B(dOut, cIn, dOut->size);
		    break;
		  case ALG_RSAES_VALUE:
		    retVal = RSAES_Decode(dOut, cIn);
		    break;
		  case ALG_OAEP_VALUE:
		    retVal = OaepDecode(dOut, scheme->details.oaep.hashAlg, label, cIn);
		    break;
		  default:
		    retVal = TPM_RC_SCHEME;
		    break;
		}
	}
 Exit:
    return retVal;
}