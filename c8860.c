CompositeDeepScanLine::Data::handleDeepFrameBuffer (DeepFrameBuffer& buf,
                                                    std::vector< unsigned int > & counts,
                                                    vector< std::vector< float* > > & pointers,
                                                    const Header& header,
                                                    int start,
                                                    int end)
{
    int width=_dataWindow.size().x+1;
    size_t pixelcount = width * (end-start+1);
    pointers.resize(_channels.size());
    counts.resize(pixelcount);
    buf.insertSampleCountSlice (Slice (OPENEXR_IMF_INTERNAL_NAMESPACE::UINT,
                                (char *) (&counts[0]-_dataWindow.min.x-start*width),
                                sizeof(unsigned int),
                                sizeof(unsigned int)*width));

    pointers[0].resize(pixelcount);
    buf.insert ("Z", DeepSlice (OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT,
                                (char *)(&pointers[0][0]-_dataWindow.min.x-start*width),
                                sizeof(float *),
                                sizeof(float *)*width,
                                sizeof(float) ));

    if(_zback)
    {
        pointers[1].resize(pixelcount);
        buf.insert ("ZBack", DeepSlice (OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT,
                                        (char *)(&pointers[1][0]-_dataWindow.min.x-start*width),
                                        sizeof(float *),
                                        sizeof(float *)*width,
                                        sizeof(float) ));
    }

    pointers[2].resize(pixelcount);
    buf.insert ("A", DeepSlice (OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT,
                                (char *)(&pointers[2][0]-_dataWindow.min.x-start*width),
                                sizeof(float *),
                                sizeof(float *)*width,
                                sizeof(float) ));


    size_t i =0;
    for(FrameBuffer::ConstIterator qt  = _outputFrameBuffer.begin();
                                   qt != _outputFrameBuffer.end();
                                   qt++)
    {
        int channel_in_source = _bufferMap[i];
        if(channel_in_source>2)
        {
            // not dealt with yet (0,1,2 previously inserted)
            pointers[channel_in_source].resize(pixelcount);
            buf.insert (qt.name(),
                        DeepSlice (OPENEXR_IMF_INTERNAL_NAMESPACE::FLOAT,
                                   (char *)(&pointers[channel_in_source][0]-_dataWindow.min.x-start*width),
                                   sizeof(float *),
                                   sizeof(float *)*width,
                                   sizeof(float) ));
        }

        i++;
    }

}