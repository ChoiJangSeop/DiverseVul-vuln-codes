static s_bucket *FNAME(find_bucket)(Channel *channel, const unsigned int val)
{
    spice_extra_assert(val < (0x1U << BPC));

    return channel->_buckets_ptrs[val];
}