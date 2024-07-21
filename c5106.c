cr_input_read_char (CRInput * a_this, guint32 * a_char)
{
        enum CRStatus status = CR_OK;
        gulong consumed = 0,
                nb_bytes_left = 0;

        g_return_val_if_fail (a_this && PRIVATE (a_this) && a_char,
                              CR_BAD_PARAM_ERROR);

        if (PRIVATE (a_this)->end_of_input == TRUE)
                return CR_END_OF_INPUT_ERROR;

        nb_bytes_left = cr_input_get_nb_bytes_left (a_this);

        if (nb_bytes_left < 1) {
                return CR_END_OF_INPUT_ERROR;
        }

        status = cr_utils_read_char_from_utf8_buf
                (PRIVATE (a_this)->in_buf
                 +
                 PRIVATE (a_this)->next_byte_index,
                 nb_bytes_left, a_char, &consumed);

        if (status == CR_OK) {
                /*update next byte index */
                PRIVATE (a_this)->next_byte_index += consumed;

                /*update line and column number */
                if (PRIVATE (a_this)->end_of_line == TRUE) {
                        PRIVATE (a_this)->col = 1;
                        PRIVATE (a_this)->line++;
                        PRIVATE (a_this)->end_of_line = FALSE;
                } else if (*a_char != '\n') {
                        PRIVATE (a_this)->col++;
                }

                if (*a_char == '\n') {
                        PRIVATE (a_this)->end_of_line = TRUE;
                }

        }

        return status;
}