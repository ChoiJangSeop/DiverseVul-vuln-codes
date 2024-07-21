ossl_x509name_cmp(VALUE self, VALUE other)
{
    int result;

    result = ossl_x509name_cmp0(self, other);
    if (result < 0) return INT2FIX(-1);
    if (result > 1) return INT2FIX(1);

    return INT2FIX(0);
}