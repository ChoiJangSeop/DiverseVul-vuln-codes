TPM2B_ID_OBJECT_Marshal(TPM2B_ID_OBJECT *source, BYTE **buffer, INT32 *size)
{
    UINT16 written = 0;
    written += TPM2B_Marshal(&source->b, buffer, size);
    return written;
}