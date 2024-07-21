QPDFNumberTreeObjectHelper::updateMap(QPDFObjectHandle oh)
{
    if (this->m->seen.count(oh.getObjGen()))
    {
        return;
    }
    this->m->seen.insert(oh.getObjGen());
    QPDFObjectHandle nums = oh.getKey("/Nums");
    if (nums.isArray())
    {
        size_t nitems = nums.getArrayNItems();
        size_t i = 0;
        while (i < nitems - 1)
        {
            QPDFObjectHandle num = nums.getArrayItem(i);
            if (num.isInteger())
            {
                ++i;
                QPDFObjectHandle obj = nums.getArrayItem(i);
                this->m->entries[num.getIntValue()] = obj;
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