QPDF::getObjectCount()
{
    // This method returns the next available indirect object number.
    // makeIndirectObject uses it for this purpose. After
    // fixDanglingReferences is called, all objects in the xref table
    // will also be in obj_cache.
    fixDanglingReferences();
    QPDFObjGen og(0, 0);
    if (! this->m->obj_cache.empty())
    {
	og = (*(this->m->obj_cache.rbegin())).first;
    }
    return og.getObj();
}