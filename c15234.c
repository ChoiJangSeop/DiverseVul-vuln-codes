SecureRandomDataProvider::provideRandomData(unsigned char* data, size_t len)
{
    throw std::logic_error("SecureRandomDataProvider::provideRandomData called when support was not compiled in");
}