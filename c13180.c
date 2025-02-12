av_cold int ff_vc2enc_init_transforms(VC2TransformContext *s, int p_width, int p_height)
{
    s->vc2_subband_dwt[VC2_TRANSFORM_9_7]    = vc2_subband_dwt_97;
    s->vc2_subband_dwt[VC2_TRANSFORM_5_3]    = vc2_subband_dwt_53;

    s->buffer = av_malloc(2*p_width*p_height*sizeof(dwtcoef));
    if (!s->buffer)
        return 1;

    return 0;
}