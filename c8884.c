ScanLineInputFile::ScanLineInputFile(InputPartData* part)
{
    if (part->header.type() != SCANLINEIMAGE)
        throw IEX_NAMESPACE::ArgExc("Can't build a ScanLineInputFile from a type-mismatched part.");

    _data = new Data(part->numThreads);
    _streamData = part->mutex;
    _data->memoryMapped = _streamData->is->isMemoryMapped();

    _data->version = part->version;

    initialize(part->header);

    _data->lineOffsets = part->chunkOffsets;

    _data->partNumber = part->partNumber;
    //
    // (TODO) change this code later.
    // The completeness of the file should be detected in MultiPartInputFile.
    //
    _data->fileIsComplete = true;
}