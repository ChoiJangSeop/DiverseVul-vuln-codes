ImageProvider::provideStreamData(int objid, int generation,
                                 Pipeline* pipeline)
{
    std::vector<PointerHolder<Pipeline> > to_delete;
    Pipeline* p = pipeline;

    if (filter == "/DCTDecode")
    {
        p = new Pl_DCT(
            "image encoder", pipeline,
            width, getHeight(), stripes[0].length(), j_color_space);
        to_delete.push_back(p);
    }
    else if (filter == "/RunLengthDecode")
    {
        p = new Pl_RunLength(
            "image encoder", pipeline, Pl_RunLength::a_encode);
        to_delete.push_back(p);
    }

    for (int i = 0; i < n_stripes; ++i)
    {
        for (int j = 0; j < width * stripe_height; ++j)
        {
            p->write(
                QUtil::unsigned_char_pointer(stripes[i].c_str()),
                stripes[i].length());
        }
    }
    p->finish();
}