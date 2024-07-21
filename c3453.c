X509_NAME::X509_NAME(const char* n, size_t sz)
    : name_(0), sz_(sz)
{
    if (sz) {
        name_ = NEW_YS char[sz];
        memcpy(name_, n, sz);
    }
    entry_.data = 0;
}