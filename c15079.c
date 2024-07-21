print_buffer(Pl_Buffer* bp)
{
    bp->finish();
    Buffer* b = bp->getBuffer();
    unsigned char const* p = b->getBuffer();
    size_t l = b->getSize();
    for (unsigned long i = 0; i < l; ++i)
    {
	printf("%02x%s", static_cast<unsigned int>(p[i]),
	       (i == l - 1) ? "\n" : " ");
    }
    printf("\n");
    delete b;
}