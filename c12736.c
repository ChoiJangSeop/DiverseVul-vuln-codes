static int cine_read_header(AVFormatContext *avctx)
{
    AVIOContext *pb = avctx->pb;
    AVStream *st;
    unsigned int version, compression, offImageHeader, offSetup, offImageOffsets, biBitCount, length, CFA;
    int vflip;
    char *description;
    uint64_t i;

    st = avformat_new_stream(avctx, NULL);
    if (!st)
        return AVERROR(ENOMEM);
    st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codecpar->codec_id   = AV_CODEC_ID_RAWVIDEO;
    st->codecpar->codec_tag  = 0;

    /* CINEFILEHEADER structure */
    avio_skip(pb, 4); // Type, Headersize

    compression = avio_rl16(pb);
    version     = avio_rl16(pb);
    if (version != 1) {
        avpriv_request_sample(avctx, "unknown version %i", version);
        return AVERROR_INVALIDDATA;
    }

    avio_skip(pb, 12); // FirstMovieImage, TotalImageCount, FirstImageNumber

    st->duration    = avio_rl32(pb);
    offImageHeader  = avio_rl32(pb);
    offSetup        = avio_rl32(pb);
    offImageOffsets = avio_rl32(pb);

    avio_skip(pb, 8); // TriggerTime

    /* BITMAPINFOHEADER structure */
    avio_seek(pb, offImageHeader, SEEK_SET);
    avio_skip(pb, 4); //biSize
    st->codecpar->width      = avio_rl32(pb);
    st->codecpar->height     = avio_rl32(pb);

    if (avio_rl16(pb) != 1) // biPlanes
        return AVERROR_INVALIDDATA;

    biBitCount = avio_rl16(pb);
    if (biBitCount != 8 && biBitCount != 16 && biBitCount != 24 && biBitCount != 48) {
        avpriv_request_sample(avctx, "unsupported biBitCount %i", biBitCount);
        return AVERROR_INVALIDDATA;
    }

    switch (avio_rl32(pb)) {
    case BMP_RGB:
        vflip = 0;
        break;
    case 0x100: /* BI_PACKED */
        st->codecpar->codec_tag = MKTAG('B', 'I', 'T', 0);
        vflip = 1;
        break;
    default:
        avpriv_request_sample(avctx, "unknown bitmap compression");
        return AVERROR_INVALIDDATA;
    }

    avio_skip(pb, 4); // biSizeImage

    /* parse SETUP structure */
    avio_seek(pb, offSetup, SEEK_SET);
    avio_skip(pb, 140); // FrameRatae16 .. descriptionOld
    if (avio_rl16(pb) != 0x5453)
        return AVERROR_INVALIDDATA;
    length = avio_rl16(pb);
    if (length < 0x163C) {
        avpriv_request_sample(avctx, "short SETUP header");
        return AVERROR_INVALIDDATA;
    }

    avio_skip(pb, 616); // Binning .. bFlipH
    if (!avio_rl32(pb) ^ vflip) {
        st->codecpar->extradata  = av_strdup("BottomUp");
        st->codecpar->extradata_size  = 9;
    }

    avio_skip(pb, 4); // Grid

    avpriv_set_pts_info(st, 64, 1, avio_rl32(pb));

    avio_skip(pb, 20); // Shutter .. bEnableColor

    set_metadata_int(&st->metadata, "camera_version", avio_rl32(pb), 0);
    set_metadata_int(&st->metadata, "firmware_version", avio_rl32(pb), 0);
    set_metadata_int(&st->metadata, "software_version", avio_rl32(pb), 0);
    set_metadata_int(&st->metadata, "recording_timezone", avio_rl32(pb), 0);

    CFA = avio_rl32(pb);

    set_metadata_int(&st->metadata, "brightness", avio_rl32(pb), 1);
    set_metadata_int(&st->metadata, "contrast", avio_rl32(pb), 1);
    set_metadata_int(&st->metadata, "gamma", avio_rl32(pb), 1);

    avio_skip(pb, 12 + 16); // Reserved1 .. AutoExpRect
    set_metadata_float(&st->metadata, "wbgain[0].r", av_int2float(avio_rl32(pb)), 1);
    set_metadata_float(&st->metadata, "wbgain[0].b", av_int2float(avio_rl32(pb)), 1);
    avio_skip(pb, 36); // WBGain[1].. WBView

    st->codecpar->bits_per_coded_sample = avio_rl32(pb);

    if (compression == CC_RGB) {
        if (biBitCount == 8) {
            st->codecpar->format = AV_PIX_FMT_GRAY8;
        } else if (biBitCount == 16) {
            st->codecpar->format = AV_PIX_FMT_GRAY16LE;
        } else if (biBitCount == 24) {
            st->codecpar->format = AV_PIX_FMT_BGR24;
        } else if (biBitCount == 48) {
            st->codecpar->format = AV_PIX_FMT_BGR48LE;
        } else {
            avpriv_request_sample(avctx, "unsupported biBitCount %i", biBitCount);
            return AVERROR_INVALIDDATA;
        }
    } else if (compression == CC_UNINT) {
        switch (CFA & 0xFFFFFF) {
        case CFA_BAYER:
            if (biBitCount == 8) {
                st->codecpar->format = AV_PIX_FMT_BAYER_GBRG8;
            } else if (biBitCount == 16) {
                st->codecpar->format = AV_PIX_FMT_BAYER_GBRG16LE;
            } else {
                avpriv_request_sample(avctx, "unsupported biBitCount %i", biBitCount);
                return AVERROR_INVALIDDATA;
            }
            break;
        case CFA_BAYERFLIP:
            if (biBitCount == 8) {
                st->codecpar->format = AV_PIX_FMT_BAYER_RGGB8;
            } else if (biBitCount == 16) {
                st->codecpar->format = AV_PIX_FMT_BAYER_RGGB16LE;
            } else {
                avpriv_request_sample(avctx, "unsupported biBitCount %i", biBitCount);
                return AVERROR_INVALIDDATA;
            }
            break;
        default:
           avpriv_request_sample(avctx, "unsupported Color Field Array (CFA) %i", CFA & 0xFFFFFF);
            return AVERROR_INVALIDDATA;
        }
    } else { //CC_LEAD
        avpriv_request_sample(avctx, "unsupported compression %i", compression);
        return AVERROR_INVALIDDATA;
    }

    avio_skip(pb, 668); // Conv8Min ... Sensor

    set_metadata_int(&st->metadata, "shutter_ns", avio_rl32(pb), 0);

    avio_skip(pb, 24); // EDRShutterNs ... ImHeightAcq

#define DESCRIPTION_SIZE 4096
    description = av_malloc(DESCRIPTION_SIZE + 1);
    if (!description)
        return AVERROR(ENOMEM);
    i = avio_get_str(pb, DESCRIPTION_SIZE, description, DESCRIPTION_SIZE + 1);
    if (i < DESCRIPTION_SIZE)
        avio_skip(pb, DESCRIPTION_SIZE - i);
    if (description[0])
        av_dict_set(&st->metadata, "description", description, AV_DICT_DONT_STRDUP_VAL);
    else
        av_free(description);

    avio_skip(pb, 1176); // RisingEdge ... cmUser

    set_metadata_int(&st->metadata, "enable_crop", avio_rl32(pb), 1);
    set_metadata_int(&st->metadata, "crop_left", avio_rl32(pb), 1);
    set_metadata_int(&st->metadata, "crop_top", avio_rl32(pb), 1);
    set_metadata_int(&st->metadata, "crop_right", avio_rl32(pb), 1);
    set_metadata_int(&st->metadata, "crop_bottom", avio_rl32(pb), 1);

    /* parse image offsets */
    avio_seek(pb, offImageOffsets, SEEK_SET);
    for (i = 0; i < st->duration; i++)
        av_add_index_entry(st, avio_rl64(pb), i, 0, 0, AVINDEX_KEYFRAME);

    return 0;
}