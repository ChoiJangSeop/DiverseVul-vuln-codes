ImageProvider::provideStreamData(int objid, int generation,
                                 Pipeline* pipeline)
{
    if (buf == 0)
    {
        buf = new unsigned char[width * stripesize];
    }
    std::cout << "page " << n << " of " << npages << std::endl;
    for (int y = 0; y < nstripes; ++y)
    {
        unsigned char color = get_pixel_color(n, y);
        memset(buf, color, width * stripesize);
        pipeline->write(buf, width * stripesize);
    }
    pipeline->finish();
}