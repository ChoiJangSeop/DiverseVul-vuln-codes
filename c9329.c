static int gset_decode_next_bit(gset_decoder *decoder)
{
    if (++decoder->bit >= 8) {
        if (++decoder->offset >= decoder->datalen) {
            return -1;
        }
        decoder->bit = 0;
    }
    return (decoder->data[decoder->offset] & cbit_mask[decoder->bit])? 1 : 0;
}