proto_register_usb_hid(void)
{
    static hf_register_info hf[] = {
        { &hf_usb_hid_item_bSize,
            { "bSize", "usbhid.item.bSize", FT_UINT8, BASE_DEC,
                VALS(usb_hid_item_bSize_vals), USBHID_SIZE_MASK, NULL, HFILL }},

        { &hf_usb_hid_item_bType,
            { "bType", "usbhid.item.bType", FT_UINT8, BASE_DEC,
                VALS(usb_hid_item_bType_vals), USBHID_TYPE_MASK, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bTag,
            { "bTag", "usbhid.item.bTag", FT_UINT8, BASE_HEX,
                VALS(usb_hid_mainitem_bTag_vals), USBHID_TAG_MASK, NULL, HFILL }},

        { &hf_usb_hid_globalitem_bTag,
            { "bTag", "usbhid.item.bTag", FT_UINT8, BASE_HEX,
                VALS(usb_hid_globalitem_bTag_vals), USBHID_TAG_MASK, NULL, HFILL }},

        { &hf_usb_hid_localitem_bTag,
            { "bTag", "usbhid.item.bTag", FT_UINT8, BASE_HEX,
                VALS(usb_hid_localitem_bTag_vals), USBHID_TAG_MASK, NULL, HFILL }},

        { &hf_usb_hid_longitem_bTag,
            { "bTag", "usbhid.item.bTag", FT_UINT8, BASE_HEX,
                VALS(usb_hid_longitem_bTag_vals), USBHID_TAG_MASK, NULL, HFILL }},

        { &hf_usb_hid_item_bDataSize,
            { "bDataSize", "usbhid.item.bDataSize", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_item_bLongItemTag,
            { "bTag", "usbhid.item.bLongItemTag", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        /* Main-report item data */

        { &hf_usb_hid_mainitem_bit0,
            { "Data/constant", "usbhid.item.main.readonly", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit0), 1<<0, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit1,
            { "Data type", "usbhid.item.main.variable", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit1), 1<<1, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit2,
            { "Coordinates", "usbhid.item.main.relative", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit2), 1<<2, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit3,
            { "Min/max wraparound", "usbhid.item.main.wrap", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit3), 1<<3, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit4,
            { "Physical relationship to data", "usbhid.item.main.nonlinear", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit4), 1<<4, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit5,
            { "Preferred state", "usbhid.item.main.no_preferred_state", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit5), 1<<5, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit6,
            { "Has null position", "usbhid.item.main.nullstate", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit6), 1<<6, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit7,
            { "(Non)-volatile", "usbhid.item.main.volatile", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit7), 1<<7, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit7_input,
            { "[Reserved]", "usbhid.item.main.volatile", FT_BOOLEAN, 9,
                NULL, 1<<7, NULL, HFILL }},

        { &hf_usb_hid_mainitem_bit8,
            { "Bits or bytes", "usbhid.item.main.buffered_bytes", FT_BOOLEAN, 9,
                TFS(&tfs_mainitem_bit8), 1<<8, NULL, HFILL }},

        { &hf_usb_hid_mainitem_colltype,
            { "Collection type", "usbhid.item.main.colltype", FT_UINT8, BASE_RANGE_STRING|BASE_HEX,
                RVALS(usb_hid_mainitem_colltype_vals), 0, NULL, HFILL }},

        /* Global-report item data */

        { &hf_usb_hid_globalitem_usage,
            { "Usage page", "usbhid.item.global.usage", FT_UINT8, BASE_RANGE_STRING|BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_log_min,
            { "Logical minimum", "usbhid.item.global.log_min", FT_INT32, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_log_max,
            { "Logical maximum", "usbhid.item.global.log_max", FT_INT32, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_phy_min,
            { "Physical minimum", "usbhid.item.global.phy_min", FT_INT32, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_phy_max,
            { "Physical maximum", "usbhid.item.global.phy_max", FT_INT32, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_exp,
            { "Unit exponent", "usbhid.item.global.unit_exp", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_sys,
            { "System", "usbhid.item.global.unit.system", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x0000000F, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_len,
            { "Length", "usbhid.item.global.unit.length", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x000000F0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_mass,
            { "Mass", "usbhid.item.global.unit.mass", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x00000F00, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_time,
            { "Time", "usbhid.item.global.unit.time", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x0000F000, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_temp,
            { "Temperature", "usbhid.item.global.unit.temperature", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x000F0000, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_current,
            { "Current", "usbhid.item.global.unit.current", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x00F00000, NULL, HFILL }},

        { &hf_usb_hid_globalitem_unit_brightness,
            { "Luminous intensity", "usbhid.item.global.unit.brightness", FT_UINT32, BASE_HEX,
                VALS(usb_hid_globalitem_unit_exp_vals), 0x0F000000, NULL, HFILL }},

        { &hf_usb_hid_globalitem_report_size,
            { "Report size", "usbhid.item.global.report_size", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_report_id,
            { "Report ID", "usbhid.item.global.report_id", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_report_count,
            { "Report count", "usbhid.item.global.report_count", FT_UINT8, BASE_DEC,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_push,
            { "Push", "usbhid.item.global.push", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_globalitem_pop,
            { "Pop", "usbhid.item.global.pop", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        /* Local-report item data */

        { &hf_usb_hid_localitem_usage,
            { "Usage", "usbhid.item.local.usage", FT_UINT8, BASE_RANGE_STRING|BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_usage_min,
            { "Usage minimum", "usbhid.item.local.usage_min", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

#if 0
        { &hf_usb_hid_localitem_usage_max,
            { "Usage maximum", "usbhid.item.local.usage_max", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},
#endif

        { &hf_usb_hid_localitem_desig_index,
            { "Designator index", "usbhid.item.local.desig_index", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_desig_min,
            { "Designator minimum", "usbhid.item.local.desig_min", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_desig_max,
            { "Designator maximum", "usbhid.item.local.desig_max", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_string_index,
            { "String index", "usbhid.item.local.string_index", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_string_min,
            { "String minimum", "usbhid.item.local.string_min", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_string_max,
            { "String maximum", "usbhid.item.local.string_max", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},

        { &hf_usb_hid_localitem_delimiter,
            { "Delimiter", "usbhid.item.local.delimiter", FT_UINT8, BASE_HEX,
                NULL, 0, NULL, HFILL }},


        { &hf_usb_hid_item_unk_data,
            { "Item data", "usbhid.item.data", FT_BYTES, BASE_NONE,
                NULL, 0, NULL, HFILL }},

        /* USB HID specific requests */
        { &hf_usb_hid_request,
            { "bRequest", "usbhid.setup.bRequest", FT_UINT8, BASE_HEX,
                VALS(setup_request_names_vals), 0x0, NULL, HFILL }},

        { &hf_usb_hid_value,
            { "wValue", "usbhid.setup.wValue", FT_UINT16, BASE_HEX,
                NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_index,
            { "wIndex", "usbhid.setup.wIndex", FT_UINT16, BASE_DEC,
                NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_length,
            { "wLength", "usbhid.setup.wLength", FT_UINT16, BASE_DEC,
                NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_report_type,
            { "ReportType", "usbhid.setup.ReportType", FT_UINT8, BASE_DEC,
                VALS(usb_hid_report_type_vals), 0x0, NULL, HFILL }},

        { &hf_usb_hid_report_id,
            { "ReportID", "usbhid.setup.ReportID", FT_UINT8, BASE_DEC,
                NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_duration,
            { "Duration", "usbhid.setup.Duration", FT_UINT8, BASE_DEC,
                NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_zero,
            { "(zero)", "usbhid.setup.zero", FT_UINT8, BASE_DEC,
                NULL, 0x0, NULL, HFILL }},

        /* components of the HID descriptor */
        { &hf_usb_hid_bcdHID,
            { "bcdHID", "usbhid.descriptor.hid.bcdHID", FT_UINT16, BASE_HEX,
                NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_bCountryCode,
            { "bCountryCode", "usbhid.descriptor.hid.bCountryCode", FT_UINT8, BASE_HEX,
                VALS(hid_country_code_vals), 0x0, NULL, HFILL }},

        { &hf_usb_hid_bNumDescriptors,
            { "bNumDescriptors", "usbhid.descriptor.hid.bNumDescriptors", FT_UINT8, BASE_DEC,
                NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_bDescriptorIndex,
            { "bDescriptorIndex", "usbhid.descriptor.hid.bDescriptorIndex", FT_UINT8, BASE_HEX,
                NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_bDescriptorType,
            { "bDescriptorType", "usbhid.descriptor.hid.bDescriptorType", FT_UINT8, BASE_HEX|BASE_EXT_STRING,
                &hid_descriptor_type_vals_ext, 0x00, NULL, HFILL }},

        { &hf_usb_hid_wInterfaceNumber,
            { "wInterfaceNumber", "usbhid.descriptor.hid.wInterfaceNumber", FT_UINT16, BASE_DEC,
                NULL, 0x0, NULL, HFILL }},

        { &hf_usb_hid_wDescriptorLength,
            { "wDescriptorLength", "usbhid.descriptor.hid.wDescriptorLength", FT_UINT16, BASE_DEC,
                NULL, 0x0, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_reserved,
            { "Reserved", "usbhid.boot_report.keyboard.reserved", FT_UINT8, BASE_HEX,
                NULL, 0x00, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_keycode_1,
            { "Keycode 1", "usbhid.boot_report.keyboard.keycode_1", FT_UINT8, BASE_HEX|BASE_EXT_STRING,
                &keycode_vals_ext, 0x00, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_keycode_2,
            { "Keycode 2", "usbhid.boot_report.keyboard.keycode_2", FT_UINT8, BASE_HEX|BASE_EXT_STRING,
                &keycode_vals_ext, 0x00, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_keycode_3,
            { "Keycode 3", "usbhid.boot_report.keyboard.keycode_3", FT_UINT8, BASE_HEX|BASE_EXT_STRING,
                &keycode_vals_ext, 0x00, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_keycode_4,
            { "Keycode 4", "usbhid.boot_report.keyboard.keycode_4", FT_UINT8, BASE_HEX|BASE_EXT_STRING,
                &keycode_vals_ext, 0x00, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_keycode_5,
            { "Keycode 5", "usbhid.boot_report.keyboard.keycode_5", FT_UINT8, BASE_HEX|BASE_EXT_STRING,
                &keycode_vals_ext, 0x00, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_keycode_6,
            { "Keycode 6", "usbhid.boot_report.keyboard.keycode_6", FT_UINT8, BASE_HEX|BASE_EXT_STRING,
                &keycode_vals_ext, 0x00, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_modifier_right_gui,
            { "Modifier: RIGHT GUI", "usbhid.boot_report.keyboard.modifier.right_gui", FT_BOOLEAN, 8,
                NULL, 0x80, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_modifier_right_alt,
            { "Modifier: RIGHT ALT", "usbhid.boot_report.keyboard.modifier.right_alt", FT_BOOLEAN, 8,
                NULL, 0x40, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_modifier_right_shift,
            { "Modifier: RIGHT SHIFT", "usbhid.boot_report.keyboard.modifier.right_shift", FT_BOOLEAN, 8,
                NULL, 0x20, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_modifier_right_ctrl,
            { "Modifier: RIGHT CTRL", "usbhid.boot_report.keyboard.modifier.right_ctrl", FT_BOOLEAN, 8,
                NULL, 0x10,NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_modifier_left_gui,
            { "Modifier: LEFT GUI", "usbhid.boot_report.keyboard.modifier.left_gui", FT_BOOLEAN, 8,
                NULL, 0x08, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_modifier_left_alt,
            { "Modifier: LEFT ALT", "usbhid.boot_report.keyboard.modifier.left_alt", FT_BOOLEAN, 8,
                NULL, 0x04, NULL, HFILL }
        },

        { &hf_usbhid_boot_report_keyboard_modifier_left_shift,
            { "Modifier: LEFT SHIFT", "usbhid.boot_report.keyboard.modifier.left_shift", FT_BOOLEAN, 8,
                NULL, 0x02, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_modifier_left_ctrl,
            { "Modifier: LEFT CTRL", "usbhid.boot_report.keyboard.modifier.left_ctrl", FT_BOOLEAN, 8,
                NULL, 0x01, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_leds_constants,
            { "Constants", "usbhid.boot_report.keyboard.leds.constants", FT_UINT8, BASE_HEX,
                NULL, 0xE0, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_leds_kana,
            { "KANA", "usbhid.boot_report.keyboard.leds.kana", FT_BOOLEAN, 8,
                NULL, 0x10, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_leds_compose,
            { "COMPOSE", "usbhid.boot_report.keyboard.leds.compose", FT_BOOLEAN, 8,
                NULL, 0x08, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_leds_scroll_lock,
            { "SCROLL LOCK", "usbhid.boot_report.keyboard.leds.scroll_lock", FT_BOOLEAN, 8,
                NULL, 0x04, NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_leds_caps_lock,
            { "CAPS LOCK", "usbhid.boot_report.keyboard.leds.caps_lock", FT_BOOLEAN, 8,
                NULL, 0x02,NULL, HFILL }},

        { &hf_usbhid_boot_report_keyboard_leds_num_lock,
            { "NUM LOCK",  "usbhid.boot_report.keyboard.leds.num_lock", FT_BOOLEAN, 8,
                NULL, 0x01, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_button_8,
            { "Button 8",  "usbhid.boot_report.mouse.button.8", FT_BOOLEAN, 8,
                NULL, 0x80, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_button_7,
            { "Button 7",  "usbhid.boot_report.mouse.button.7", FT_BOOLEAN, 8,
                NULL, 0x40, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_button_6,
            { "Button 6",  "usbhid.boot_report.mouse.button.6", FT_BOOLEAN, 8,
                NULL, 0x20, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_button_5,
            { "Button 5",  "usbhid.boot_report.mouse.button.5", FT_BOOLEAN, 8,
                NULL, 0x10, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_button_4,
            { "Button 4",  "usbhid.boot_report.mouse.button.4", FT_BOOLEAN, 8,
                NULL, 0x08, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_button_middle,
            { "Button Middle", "usbhid.boot_report.mouse.button.middle", FT_BOOLEAN, 8,
                NULL, 0x04, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_button_right,
            { "Button Right",  "usbhid.boot_report.mouse.button.right", FT_BOOLEAN, 8,
                NULL, 0x02, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_button_left,
            { "Button Left",   "usbhid.boot_report.mouse.button.left", FT_BOOLEAN, 8,
                NULL, 0x01, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_x_displacement,
            { "X Displacement", "usbhid.boot_report.mouse.x_displacement", FT_INT8, BASE_DEC,
                NULL, 0x00, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_y_displacement,
            { "Y Displacement", "usbhid.boot_report.mouse.y_displacement", FT_INT8, BASE_DEC,
                NULL, 0x00, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_horizontal_scroll_wheel,
            { "Horizontal Scroll Wheel", "usbhid.boot_report.mouse.scroll_wheel.horizontal", FT_INT8, BASE_DEC,
                NULL, 0x00, NULL, HFILL }},

        { &hf_usbhid_boot_report_mouse_vertical_scroll_wheel,
            { "Vertical Scroll Wheel", "usbhid.boot_report.mouse.scroll_wheel.vertical", FT_INT8, BASE_DEC,
                NULL, 0x00, NULL, HFILL }},

        { &hf_usbhid_data,
            { "HID Data", "usbhid.data", FT_BYTES, BASE_NONE,
                NULL, 0x00, NULL, HFILL }},
    };

    static gint *usb_hid_subtrees[] = {
        &ett_usb_hid_report,
        &ett_usb_hid_item_header,
        &ett_usb_hid_wValue,
        &ett_usb_hid_descriptor,
        &ett_usb_hid_data
    };

    report_descriptors = wmem_tree_new_autoreset(wmem_epan_scope(), wmem_file_scope());

    proto_usb_hid = proto_register_protocol("USB HID", "USBHID", "usbhid");
    proto_register_field_array(proto_usb_hid, hf, array_length(hf));
    proto_register_subtree_array(usb_hid_subtrees, array_length(usb_hid_subtrees));

    /*usb_hid_boot_keyboard_input_report_handle  =*/ register_dissector("usbhid.boot_report.keyboard.input",  dissect_usb_hid_boot_keyboard_input_report,  proto_usb_hid);
    /*usb_hid_boot_keyboard_output_report_handle =*/ register_dissector("usbhid.boot_report.keyboard.output", dissect_usb_hid_boot_keyboard_output_report, proto_usb_hid);
    /*usb_hid_boot_mouse_input_report_handle     =*/ register_dissector("usbhid.boot_report.mouse.input",     dissect_usb_hid_boot_mouse_input_report,     proto_usb_hid);

}