static void rpza_decode_stream(RpzaContext *s)
{
    int width = s->avctx->width;
    int stride = s->frame->linesize[0] / 2;
    int row_inc = stride - 4;
    int chunk_size;
    uint16_t colorA = 0, colorB;
    uint16_t color4[4];
    uint16_t ta, tb;
    uint16_t *pixels = (uint16_t *)s->frame->data[0];

    int row_ptr = 0;
    int pixel_ptr = 0;
    int block_ptr;
    int pixel_x, pixel_y;
    int total_blocks;

    /* First byte is always 0xe1. Warn if it's different */
    if (bytestream2_peek_byte(&s->gb) != 0xe1)
        av_log(s->avctx, AV_LOG_ERROR, "First chunk byte is 0x%02x instead of 0xe1\n",
               bytestream2_peek_byte(&s->gb));

    /* Get chunk size, ingnoring first byte */
    chunk_size = bytestream2_get_be32(&s->gb) & 0x00FFFFFF;

    /* If length mismatch use size from MOV file and try to decode anyway */
    if (chunk_size != bytestream2_get_bytes_left(&s->gb) - 4)
        av_log(s->avctx, AV_LOG_WARNING, "MOV chunk size != encoded chunk size\n");

    /* Number of 4x4 blocks in frame. */
    total_blocks = ((s->avctx->width + 3) / 4) * ((s->avctx->height + 3) / 4);

    /* Process chunk data */
    while (bytestream2_get_bytes_left(&s->gb)) {
        uint8_t opcode = bytestream2_get_byte(&s->gb); /* Get opcode */

        int n_blocks = (opcode & 0x1f) + 1; /* Extract block counter from opcode */

        /* If opcode MSbit is 0, we need more data to decide what to do */
        if ((opcode & 0x80) == 0) {
            colorA = (opcode << 8) | bytestream2_get_byte(&s->gb);
            opcode = 0;
            if ((bytestream2_peek_byte(&s->gb) & 0x80) != 0) {
                /* Must behave as opcode 110xxxxx, using colorA computed
                 * above. Use fake opcode 0x20 to enter switch block at
                 * the right place */
                opcode = 0x20;
                n_blocks = 1;
            }
        }

        switch (opcode & 0xe0) {

        /* Skip blocks */
        case 0x80:
            while (n_blocks--) {
              ADVANCE_BLOCK();
            }
            break;

        /* Fill blocks with one color */
        case 0xa0:
            colorA = bytestream2_get_be16(&s->gb);
            while (n_blocks--) {
                block_ptr = row_ptr + pixel_ptr;
                for (pixel_y = 0; pixel_y < 4; pixel_y++) {
                    for (pixel_x = 0; pixel_x < 4; pixel_x++){
                        pixels[block_ptr] = colorA;
                        block_ptr++;
                    }
                    block_ptr += row_inc;
                }
                ADVANCE_BLOCK();
            }
            break;

        /* Fill blocks with 4 colors */
        case 0xc0:
            colorA = bytestream2_get_be16(&s->gb);
        case 0x20:
            colorB = bytestream2_get_be16(&s->gb);

            /* sort out the colors */
            color4[0] = colorB;
            color4[1] = 0;
            color4[2] = 0;
            color4[3] = colorA;

            /* red components */
            ta = (colorA >> 10) & 0x1F;
            tb = (colorB >> 10) & 0x1F;
            color4[1] |= ((11 * ta + 21 * tb) >> 5) << 10;
            color4[2] |= ((21 * ta + 11 * tb) >> 5) << 10;

            /* green components */
            ta = (colorA >> 5) & 0x1F;
            tb = (colorB >> 5) & 0x1F;
            color4[1] |= ((11 * ta + 21 * tb) >> 5) << 5;
            color4[2] |= ((21 * ta + 11 * tb) >> 5) << 5;

            /* blue components */
            ta = colorA & 0x1F;
            tb = colorB & 0x1F;
            color4[1] |= ((11 * ta + 21 * tb) >> 5);
            color4[2] |= ((21 * ta + 11 * tb) >> 5);

            if (bytestream2_get_bytes_left(&s->gb) < n_blocks * 4)
                return;
            while (n_blocks--) {
                block_ptr = row_ptr + pixel_ptr;
                for (pixel_y = 0; pixel_y < 4; pixel_y++) {
                    uint8_t index = bytestream2_get_byteu(&s->gb);
                    for (pixel_x = 0; pixel_x < 4; pixel_x++){
                        uint8_t idx = (index >> (2 * (3 - pixel_x))) & 0x03;
                        pixels[block_ptr] = color4[idx];
                        block_ptr++;
                    }
                    block_ptr += row_inc;
                }
                ADVANCE_BLOCK();
            }
            break;

        /* Fill block with 16 colors */
        case 0x00:
            if (bytestream2_get_bytes_left(&s->gb) < 30)
                return;
            block_ptr = row_ptr + pixel_ptr;
            for (pixel_y = 0; pixel_y < 4; pixel_y++) {
                for (pixel_x = 0; pixel_x < 4; pixel_x++){
                    /* We already have color of upper left pixel */
                    if ((pixel_y != 0) || (pixel_x != 0))
                        colorA = bytestream2_get_be16u(&s->gb);
                    pixels[block_ptr] = colorA;
                    block_ptr++;
                }
                block_ptr += row_inc;
            }
            ADVANCE_BLOCK();
            break;

        /* Unknown opcode */
        default:
            av_log(s->avctx, AV_LOG_ERROR, "Unknown opcode %d in rpza chunk."
                 " Skip remaining %d bytes of chunk data.\n", opcode,
                 bytestream2_get_bytes_left(&s->gb));
            return;
        } /* Opcode switch */
    }
}