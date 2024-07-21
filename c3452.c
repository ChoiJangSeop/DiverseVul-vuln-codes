ASN1_STRING* X509_NAME::GetEntry(int i)
{
    if (i < 0 || i >= int(sz_))
        return 0;

    if (entry_.data)
        ysArrayDelete(entry_.data);
    entry_.data = NEW_YS byte[sz_];       // max size;

    memcpy(entry_.data, &name_[i], sz_ - i);
    if (entry_.data[sz_ -i - 1]) {
        entry_.data[sz_ - i] = 0;
        entry_.length = int(sz_) - i;
    }
    else
        entry_.length = int(sz_) - i - 1;
    entry_.type = 0;

    return &entry_;
}