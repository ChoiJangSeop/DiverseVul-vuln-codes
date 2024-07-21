InputFile::InputFile (InputPartData* part) :
    _data (new Data (part->numThreads))
{
    _data->_deleteStream=false;
    multiPartInitialize (part);
}