ImageInverter::provideStreamData(int objid, int generation,
				 Pipeline* pipeline)
{
    // Use the object and generation number supplied to look up the
    // image data.  Then invert the image data and write the inverted
    // data to the pipeline.
    PointerHolder<Buffer> data =
        this->image_data[QPDFObjGen(objid, generation)];
    size_t size = data->getSize();
    unsigned char* buf = data->getBuffer();
    unsigned char ch;
    for (size_t i = 0; i < size; ++i)
    {
	ch = static_cast<unsigned char>(0xff) - buf[i];
	pipeline->write(&ch, 1);
    }
    pipeline->finish();
}