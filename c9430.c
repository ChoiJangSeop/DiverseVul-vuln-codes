HttpHeader::chunked() const
{
    return has(Http::HdrType::TRANSFER_ENCODING) &&
           hasListMember(Http::HdrType::TRANSFER_ENCODING, "chunked", ',');
}