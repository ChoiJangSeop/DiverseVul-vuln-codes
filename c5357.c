QPDFObjectHandle::newIndirect(QPDF* qpdf, int objid, int generation)
{
    return QPDFObjectHandle(qpdf, objid, generation);
}