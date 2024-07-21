TPM2B_Marshal(TPM2B *source, BYTE **buffer, INT32 *size)
{
    UINT16 written = 0;
    written += UINT16_Marshal(&(source->size), buffer, size);
    written += Array_Marshal(source->buffer, source->size, buffer, size); 
    return written;
}