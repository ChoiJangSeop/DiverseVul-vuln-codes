QPDFNameTreeObjectHelper::updateMap(QPDFObjectHandle oh)
{
    if (this->m->seen.count(oh.getObjGen()))
    {
        return;
    }
    this->m->seen.insert(oh.getObjGen());
    QPDFObjectHandle names = oh.getKey("/Names");
    if (names.isArray())
    {
        size_t nitems = names.getArrayNItems();
        size_t i = 0;
        while (i < nitems - 1)
        {
            QPDFObjectHandle name = names.getArrayItem(i);
            if (name.isString())
            {
                ++i;
                QPDFObjectHandle obj = names.getArrayItem(i);
                this->m->entries[name.getUTF8Value()] = obj;
            }
            ++i;
        }
    }
    QPDFObjectHandle kids = oh.getKey("/Kids");
    if (kids.isArray())
    {
        size_t nitems = kids.getArrayNItems();
        for (size_t i = 0; i < nitems; ++i)
        {
            updateMap(kids.getArrayItem(i));
        }
    }
}