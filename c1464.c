cmsSEQ* CMSEXPORT cmsAllocProfileSequenceDescription(cmsContext ContextID, cmsUInt32Number n)
{
    cmsSEQ* Seq;
    cmsUInt32Number i;

    if (n == 0) return NULL;

    // In a absolutely arbitrary way, I hereby decide to allow a maxim of 255 profiles linked
    // in a devicelink. It makes not sense anyway and may be used for exploits, so let's close the door!
    if (n > 255) return NULL;

    Seq = (cmsSEQ*) _cmsMallocZero(ContextID, sizeof(cmsSEQ));
    if (Seq == NULL) return NULL;

    Seq -> ContextID = ContextID;
    Seq -> seq      = (cmsPSEQDESC*) _cmsCalloc(ContextID, n, sizeof(cmsPSEQDESC));
    Seq -> n        = n;


    for (i=0; i < n; i++) {
        Seq -> seq[i].Manufacturer = NULL;
        Seq -> seq[i].Model        = NULL;
        Seq -> seq[i].Description  = NULL;
    }

    return Seq;
}