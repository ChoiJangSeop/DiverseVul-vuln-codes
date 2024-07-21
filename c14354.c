struct razer_report razer_chroma_standard_matrix_set_custom_frame(unsigned char row_index, unsigned char start_col, unsigned char stop_col, unsigned char *rgb_data)
{
    size_t row_length = (size_t) (((stop_col + 1) - start_col) * 3);
    struct razer_report report = get_razer_report(0x03, 0x0B, 0x46); // In theory should be able to leave data size at max as we have start/stop

    // printk(KERN_ALERT "razerkbd: Row ID: %d, Start: %d, Stop: %d, row length: %d\n", row_index, start_col, stop_col, (unsigned char)row_length);

    report.arguments[0] = 0xFF; // Frame ID
    report.arguments[1] = row_index;
    report.arguments[2] = start_col;
    report.arguments[3] = stop_col;
    memcpy(&report.arguments[4], rgb_data, row_length);

    return report;
}