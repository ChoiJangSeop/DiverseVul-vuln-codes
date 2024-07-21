dissect_usb_hid_data(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void *data _U_)
{
    guint offset = 0;
    proto_item *hid_ti;
    proto_tree _U_ *hid_tree;
    guint remaining = tvb_reported_length_remaining(tvb, offset);

    if (remaining) {
        hid_ti = proto_tree_add_item(tree, hf_usbhid_data, tvb, offset, -1, ENC_NA);
        hid_tree = proto_item_add_subtree(hid_ti, ett_usb_hid_data);
        offset += remaining;
    }

    return offset;
}