DeepTiledInputFile::multiPartInitialize(InputPartData* part)
{
    if (isTiled(part->header.type()) == false)
        THROW (IEX_NAMESPACE::ArgExc, "Can't build a DeepTiledInputFile from a part of type " << part->header.type());

    _data->_streamData = part->mutex;
    _data->header = part->header;
    _data->version = part->version;
    _data->partNumber = part->partNumber;
    _data->memoryMapped = _data->_streamData->is->isMemoryMapped();
    initialize();
    _data->tileOffsets.readFrom(part->chunkOffsets , _data->fileIsComplete);
    _data->_streamData->currentPosition = _data->_streamData->is->tellg();
}