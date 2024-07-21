QPDF_Array::setItem(int n, QPDFObjectHandle const& oh)
{
    // Call getItem for bounds checking
    (void) getItem(n);
    this->items.at(n) = oh;
}