static apr_int32_t h2_log2inv(unsigned char log2)
{
    return log2? (1 << log2) : 1;
}