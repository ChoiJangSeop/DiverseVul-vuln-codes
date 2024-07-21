TPM2B_DIGEST_Marshal(TPM2B_DIGEST *source, BYTE **buffer, INT32 *size)
{
UINT16 written = 0;
written += TPM2B_Marshal(&source->b, buffer, size);
return written;
}