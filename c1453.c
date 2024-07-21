void CMSEXPORT cmsFreeNamedColorList(cmsNAMEDCOLORLIST* v)
{
    if (v ->List) _cmsFree(v ->ContextID, v ->List);
    if (v) _cmsFree(v ->ContextID, v);
}