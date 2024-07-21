void FreeElem(_cmsDICelem* e)
{
    if (e ->Offsets != NULL)  _cmsFree(e -> ContextID, e -> Offsets);
    if (e ->Sizes   != NULL)  _cmsFree(e -> ContextID, e ->Sizes);
    e->Offsets = e ->Sizes = NULL;
}