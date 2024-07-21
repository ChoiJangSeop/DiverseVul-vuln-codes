DeepScanLineInputFile::~DeepScanLineInputFile ()
{
    if (_data->_deleteStream)
        delete _data->_streamData->is;

    if (_data)
    {
        if (!_data->memoryMapped)
            for (size_t i = 0; i < _data->lineBuffers.size(); i++)
                delete [] _data->lineBuffers[i]->buffer;

        //
        // Unless this file was opened via the multipart API, delete the streamdata
        // object too.
        // (TODO) it should be "isMultiPart(data->version)", but when there is only
        // single part,
        // (see the above constructor) the version field is not set.
        //
        // (TODO) we should have a way to tell if the stream data is owned by this
        // file or by a parent multipart file.
        //

        if (_data->partNumber == -1 && _data->_streamData)
            delete _data->_streamData;

        delete _data;
    }
}