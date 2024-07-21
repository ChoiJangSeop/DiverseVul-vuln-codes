int WavpackPackInit (WavpackContext *wpc)
{
    if (wpc->metabytes > 16384)             // 16384 bytes still leaves plenty of room for audio
        write_metadata_block (wpc);         //  in this block (otherwise write a special one)

    // The default block size is a compromise. Longer blocks provide better encoding efficiency,
    // but longer blocks adversely affect memory requirements and seeking performance. For WavPack
    // version 5.0, the default block sizes have been reduced by half from the previous version,
    // but the difference in encoding efficiency will generally be less than 0.1 percent.

    if (wpc->dsd_multiplier) {
        wpc->block_samples = (wpc->config.sample_rate % 7) ? 48000 : 44100;

        if (wpc->config.flags & CONFIG_HIGH_FLAG)
            wpc->block_samples /= 2;

        if (wpc->config.num_channels == 1)
            wpc->block_samples *= 2;

        while (wpc->block_samples > 12000 && wpc->block_samples * wpc->config.num_channels > 300000)
            wpc->block_samples /= 2;
    }
    else {
        int divisor = (wpc->config.flags & CONFIG_HIGH_FLAG) ? 2 : 4;

        while (wpc->config.sample_rate % divisor)
            divisor--;

        wpc->block_samples = wpc->config.sample_rate / divisor;

        while (wpc->block_samples > 12000 && wpc->block_samples * wpc->config.num_channels > 75000)
            wpc->block_samples /= 2;

        while (wpc->block_samples * wpc->config.num_channels < 20000)
            wpc->block_samples *= 2;
    }

    if (wpc->config.block_samples) {
        if ((wpc->config.flags & CONFIG_MERGE_BLOCKS) &&
            wpc->block_samples > (uint32_t) wpc->config.block_samples) {
                wpc->block_boundary = wpc->config.block_samples;
                wpc->block_samples /= wpc->config.block_samples;
                wpc->block_samples *= wpc->config.block_samples;
        }
        else
            wpc->block_samples = wpc->config.block_samples;
    }

    wpc->ave_block_samples = wpc->block_samples;
    wpc->max_samples = wpc->block_samples + (wpc->block_samples >> 1);

    for (wpc->current_stream = 0; wpc->current_stream < wpc->num_streams; wpc->current_stream++) {
        WavpackStream *wps = wpc->streams [wpc->current_stream];

        wps->sample_buffer = malloc (wpc->max_samples * (wps->wphdr.flags & MONO_FLAG ? 4 : 8));

#ifdef ENABLE_DSD
        if (wps->wphdr.flags & DSD_FLAG)
            pack_dsd_init (wpc);
        else
#endif
            pack_init (wpc);
    }

    return TRUE;
}