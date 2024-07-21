void MatrixElemTypeFree(cmsStage* mpe)
{
    _cmsStageMatrixData* Data = (_cmsStageMatrixData*) mpe ->Data;
    if (Data ->Double)
        _cmsFree(mpe ->ContextID, Data ->Double);

    if (Data ->Offset)
        _cmsFree(mpe ->ContextID, Data ->Offset);

    _cmsFree(mpe ->ContextID, mpe ->Data);
}