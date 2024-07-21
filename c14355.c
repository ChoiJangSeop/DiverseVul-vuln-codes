struct razer_report razer_chroma_extended_matrix_set_custom_frame2(unsigned char row_index, unsigned char start_col, unsigned char stop_col, unsigned char *rgb_data, size_t packetLength)
{
    const size_t row_length = (size_t) (((stop_col + 1) - start_col) * 3);
    // Some devices need a specific packet length, most devices are happy with 0x47
    // e.g. the Mamba Elite needs a "row_length + 5" packet length
    const size_t data_length = (packetLength != 0) ? packetLength : row_length + 5;
    struct razer_report report = get_razer_report(0x0F, 0x03, data_length);

    report.transaction_id.id = 0x3F;

    // printk(KERN_ALERT "razerkbd: Row ID: %d, Start: %d, Stop: %d, row length: %d\n", row_index, start_col, stop_col, (unsigned char)row_length);

    report.arguments[2] = row_index;
    report.arguments[3] = start_col;
    report.arguments[4] = stop_col;
    memcpy(&report.arguments[5], rgb_data, row_length);

    return report;
}