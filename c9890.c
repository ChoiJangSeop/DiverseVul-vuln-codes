int WavpackSetConfiguration64 (WavpackContext *wpc, WavpackConfig *config, int64_t total_samples, const unsigned char *chan_ids)
{
    uint32_t flags, bps = 0;
    uint32_t chan_mask = config->channel_mask;
    int num_chans = config->num_channels;
    int i;

    if (config->sample_rate <= 0) {
        strcpy (wpc->error_message, "sample rate cannot be zero or negative!");
        return FALSE;
    }

    if (!num_chans) {
        strcpy (wpc->error_message, "channel count cannot be zero!");
        return FALSE;
    }

    wpc->stream_version = (config->flags & CONFIG_COMPATIBLE_WRITE) ? CUR_STREAM_VERS : MAX_STREAM_VERS;

    if ((config->qmode & QMODE_DSD_AUDIO) && config->bytes_per_sample == 1 && config->bits_per_sample == 8) {
#ifdef ENABLE_DSD
        wpc->dsd_multiplier = 1;
        flags = DSD_FLAG;

        for (i = 14; i >= 0; --i)
            if (config->sample_rate % sample_rates [i] == 0) {
                int divisor = config->sample_rate / sample_rates [i];

                if (divisor && (divisor & (divisor - 1)) == 0) {
                    config->sample_rate /= divisor;
                    wpc->dsd_multiplier = divisor;
                    break;
                }
            }

        // most options that don't apply to DSD we can simply ignore for now, but NOT hybrid mode!
        if (config->flags & CONFIG_HYBRID_FLAG) {
            strcpy (wpc->error_message, "hybrid mode not available for DSD!");
            return FALSE;
        }

        // with DSD, very few PCM options work (or make sense), so only allow those that do
        config->flags &= (CONFIG_HIGH_FLAG | CONFIG_MD5_CHECKSUM | CONFIG_PAIR_UNDEF_CHANS);
        config->float_norm_exp = config->xmode = 0;
#else
        strcpy (wpc->error_message, "libwavpack not configured for DSD!");
        return FALSE;
#endif
    }
    else
        flags = config->bytes_per_sample - 1;

    wpc->total_samples = total_samples;
    wpc->config.sample_rate = config->sample_rate;
    wpc->config.num_channels = config->num_channels;
    wpc->config.channel_mask = config->channel_mask;
    wpc->config.bits_per_sample = config->bits_per_sample;
    wpc->config.bytes_per_sample = config->bytes_per_sample;
    wpc->config.block_samples = config->block_samples;
    wpc->config.flags = config->flags;
    wpc->config.qmode = config->qmode;

    if (config->flags & CONFIG_VERY_HIGH_FLAG)
        wpc->config.flags |= CONFIG_HIGH_FLAG;

    for (i = 0; i < 15; ++i)
        if (wpc->config.sample_rate == sample_rates [i])
            break;

    flags |= i << SRATE_LSB;

    // all of this stuff only applies to PCM

    if (!(flags & DSD_FLAG)) {
        if (config->float_norm_exp) {
            if (config->bytes_per_sample != 4 || config->bits_per_sample != 32) {
                strcpy (wpc->error_message, "incorrect bits/bytes configuration for float data!");
                return FALSE;
            }

            wpc->config.float_norm_exp = config->float_norm_exp;
            wpc->config.flags |= CONFIG_FLOAT_DATA;
            flags |= FLOAT_DATA;
        }
        else {
            if (config->bytes_per_sample < 1 || config->bytes_per_sample > 4) {
                strcpy (wpc->error_message, "invalid bytes per sample!");
                return FALSE;
            }

            if (config->bits_per_sample < 1 || config->bits_per_sample > config->bytes_per_sample * 8) {
                strcpy (wpc->error_message, "invalid bits per sample!");
                return FALSE;
            }

            flags |= ((config->bytes_per_sample * 8) - config->bits_per_sample) << SHIFT_LSB;
        }

        if (config->flags & CONFIG_HYBRID_FLAG) {
            flags |= HYBRID_FLAG | HYBRID_BITRATE | HYBRID_BALANCE;

            if (!(wpc->config.flags & CONFIG_SHAPE_OVERRIDE)) {
                wpc->config.flags |= CONFIG_HYBRID_SHAPE | CONFIG_AUTO_SHAPING;
                flags |= HYBRID_SHAPE | NEW_SHAPING;
            }
            else if (wpc->config.flags & CONFIG_HYBRID_SHAPE) {
                wpc->config.shaping_weight = config->shaping_weight;
                flags |= HYBRID_SHAPE | NEW_SHAPING;
            }

            if (wpc->config.flags & (CONFIG_CROSS_DECORR | CONFIG_OPTIMIZE_WVC))
                flags |= CROSS_DECORR;

            if (config->flags & CONFIG_BITRATE_KBPS) {
                bps = (uint32_t) floor (config->bitrate * 256000.0 / config->sample_rate / config->num_channels + 0.5);

                if (bps > (64 << 8))
                    bps = 64 << 8;
            }
            else
                bps = (uint32_t) floor (config->bitrate * 256.0 + 0.5);
        }
        else
            flags |= CROSS_DECORR;

        if (!(config->flags & CONFIG_JOINT_OVERRIDE) || (config->flags & CONFIG_JOINT_STEREO))
            flags |= JOINT_STEREO;

        if (config->flags & CONFIG_CREATE_WVC)
            wpc->wvc_flag = TRUE;
    }

    // if a channel-identities string was specified, process that here, otherwise all channels
    // not present in the channel mask are considered "unassigned"

    if (chan_ids) {
        int lastchan = 0, mask_copy = chan_mask;

        if ((int) strlen ((char *) chan_ids) > num_chans) {          // can't be more than num channels!
            strcpy (wpc->error_message, "chan_ids longer than num channels!");
            return FALSE;
        }

        // skip past channels that are specified in the channel mask (no reason to store those)

        while (*chan_ids)
            if (*chan_ids <= 32 && *chan_ids > lastchan && (mask_copy & (1U << (*chan_ids-1)))) {
                mask_copy &= ~(1U << (*chan_ids-1));
                lastchan = *chan_ids++;
            }
            else
                break;

        // now scan the string for an actually defined channel (and don't store if there aren't any)

        for (i = 0; chan_ids [i]; i++)
            if (chan_ids [i] != 0xff) {
                wpc->channel_identities = (unsigned char *) strdup ((char *) chan_ids);
                break;
            }
    }

    // This loop goes through all the channels and creates the Wavpack "streams" for them to go in.
    // A stream can hold either one or two channels, so we have several rules to determine how many
    // channels will go in each stream.

    for (wpc->current_stream = 0; num_chans; wpc->current_stream++) {
        WavpackStream *wps = malloc (sizeof (WavpackStream));
        unsigned char left_chan_id = 0, right_chan_id = 0;
        int pos, chans = 1;

        // allocate the stream and initialize the pointer to it
        wpc->streams = realloc (wpc->streams, (wpc->current_stream + 1) * sizeof (wpc->streams [0]));
        wpc->streams [wpc->current_stream] = wps;
        CLEAR (*wps);

        // if there are any bits [still] set in the channel_mask, get the next one or two IDs from there
        if (chan_mask)
            for (pos = 0; pos < 32; ++pos)
                if (chan_mask & (1U << pos)) {
                    if (left_chan_id) {
                        right_chan_id = pos + 1;
                        break;
                    }
                    else {
                        chan_mask &= ~(1U << pos);
                        left_chan_id = pos + 1;
                    }
                }

        // next check for any channels identified in the channel-identities string
        while (!right_chan_id && chan_ids && *chan_ids)
            if (left_chan_id)
                right_chan_id = *chan_ids;
            else
                left_chan_id = *chan_ids++;

        // assume anything we did not get is "unassigned"
        if (!left_chan_id)
            left_chan_id = right_chan_id = 0xff;
        else if (!right_chan_id)
            right_chan_id = 0xff;

        // if we have 2 channels, this is where we decide if we can combine them into one stream:
        // 1. they are "unassigned" and we've been told to combine unassigned pairs, or
        // 2. they appear together in the valid "pairings" list
        if (num_chans >= 2) {
            if ((config->flags & CONFIG_PAIR_UNDEF_CHANS) && left_chan_id == 0xff && right_chan_id == 0xff)
                chans = 2;
            else
                for (i = 0; i < NUM_STEREO_PAIRS; ++i)
                    if ((left_chan_id == stereo_pairs [i].a && right_chan_id == stereo_pairs [i].b) ||
                        (left_chan_id == stereo_pairs [i].b && right_chan_id == stereo_pairs [i].a)) {
                            if (right_chan_id <= 32 && (chan_mask & (1U << (right_chan_id-1))))
                                chan_mask &= ~(1U << (right_chan_id-1));
                            else if (chan_ids && *chan_ids == right_chan_id)
                                chan_ids++;

                            chans = 2;
                            break;
                        }
        }

        num_chans -= chans;

        if (num_chans && wpc->current_stream == NEW_MAX_STREAMS - 1)
            break;

        memcpy (wps->wphdr.ckID, "wvpk", 4);
        wps->wphdr.ckSize = sizeof (WavpackHeader) - 8;
        SET_TOTAL_SAMPLES (wps->wphdr, wpc->total_samples);
        wps->wphdr.version = wpc->stream_version;
        wps->wphdr.flags = flags;
        wps->bits = bps;

        if (!wpc->current_stream)
            wps->wphdr.flags |= INITIAL_BLOCK;

        if (!num_chans)
            wps->wphdr.flags |= FINAL_BLOCK;

        if (chans == 1) {
            wps->wphdr.flags &= ~(JOINT_STEREO | CROSS_DECORR | HYBRID_BALANCE);
            wps->wphdr.flags |= MONO_FLAG;
        }
    }

    wpc->num_streams = wpc->current_stream;
    wpc->current_stream = 0;

    if (num_chans) {
        strcpy (wpc->error_message, "too many channels!");
        return FALSE;
    }

    if (config->flags & CONFIG_EXTRA_MODE)
        wpc->config.xmode = config->xmode ? config->xmode : 1;

    return TRUE;
}