cmsBool AllocElem(cmsContext ContextID, _cmsDICelem* e,  cmsUInt32Number Count)
{
    e->Offsets = (cmsUInt32Number *) _cmsCalloc(ContextID, Count, sizeof(cmsUInt32Number *));
    if (e->Offsets == NULL) return FALSE;

    e->Sizes = (cmsUInt32Number *) _cmsCalloc(ContextID, Count, sizeof(cmsUInt32Number *));
    if (e->Sizes == NULL) {

        _cmsFree(ContextID, e -> Offsets);
        return FALSE;
    }

    e ->ContextID = ContextID;
    return TRUE;
}