proto_register_kafka_expert_module(const int proto) {
    expert_module_t* expert_kafka;
    static ei_register_info ei[] = {
            { &ei_kafka_request_missing,
                    { "kafka.request_missing", PI_UNDECODED, PI_WARN, "Request missing", EXPFILL }},
            { &ei_kafka_unknown_api_key,
                    { "kafka.unknown_api_key", PI_UNDECODED, PI_WARN, "Unknown API key", EXPFILL }},
            { &ei_kafka_unsupported_api_version,
                    { "kafka.unsupported_api_version", PI_UNDECODED, PI_WARN, "Unsupported API version", EXPFILL }},
            { &ei_kafka_bad_string_length,
                    { "kafka.bad_string_length", PI_MALFORMED, PI_WARN, "Invalid string length field", EXPFILL }},
            { &ei_kafka_bad_bytes_length,
                    { "kafka.bad_bytes_length", PI_MALFORMED, PI_WARN, "Invalid byte length field", EXPFILL }},
            { &ei_kafka_bad_array_length,
                    { "kafka.bad_array_length", PI_MALFORMED, PI_WARN, "Invalid array length field", EXPFILL }},
            { &ei_kafka_bad_record_length,
                    { "kafka.bad_record_length", PI_MALFORMED, PI_WARN, "Invalid record length field", EXPFILL }},
            { &ei_kafka_bad_varint,
                    { "kafka.bad_varint", PI_MALFORMED, PI_WARN, "Invalid varint bytes", EXPFILL }},
            { &ei_kafka_bad_message_set_length,
                    { "kafka.ei_kafka_bad_message_set_length", PI_MALFORMED, PI_WARN, "Message set size does not match content", EXPFILL }},
            { &ei_kafka_unknown_message_magic,
                    { "kafka.unknown_message_magic", PI_MALFORMED, PI_WARN, "Invalid message magic field", EXPFILL }},
            { &ei_kafka_pdu_length_mismatch,
                    { "kafka.pdu_length_mismatch", PI_MALFORMED, PI_WARN, "Dissected message does not end at the pdu length offset", EXPFILL }},
    };
    expert_kafka = expert_register_protocol(proto);
    expert_register_field_array(expert_kafka, ei, array_length(ei));
}