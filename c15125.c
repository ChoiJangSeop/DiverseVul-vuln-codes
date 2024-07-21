QPDF::makeIndirectObject(QPDFObjectHandle oh)
{
    int max_objid = getObjectCount();
    QPDFObjGen next(max_objid + 1, 0);
    this->m->obj_cache[next] =
	ObjCache(QPDFObjectHandle::ObjAccessor::getObject(oh), -1, -1);
    return QPDFObjectHandle::Factory::newIndirect(
        this, next.getObj(), next.getGen());
}