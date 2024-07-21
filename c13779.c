const char* ExtensionSet::ParseMessageSetItemTmpl(
    const char* ptr, const Msg* extendee, internal::InternalMetadata* metadata,
    internal::ParseContext* ctx) {
  std::string payload;
  uint32_t type_id = 0;
  bool payload_read = false;
  while (!ctx->Done(&ptr)) {
    uint32_t tag = static_cast<uint8_t>(*ptr++);
    if (tag == WireFormatLite::kMessageSetTypeIdTag) {
      uint64_t tmp;
      ptr = ParseBigVarint(ptr, &tmp);
      GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
      type_id = tmp;
      if (payload_read) {
        ExtensionInfo extension;
        bool was_packed_on_wire;
        if (!FindExtension(2, type_id, extendee, ctx, &extension,
                           &was_packed_on_wire)) {
          WriteLengthDelimited(type_id, payload,
                               metadata->mutable_unknown_fields<T>());
        } else {
          MessageLite* value =
              extension.is_repeated
                  ? AddMessage(type_id, WireFormatLite::TYPE_MESSAGE,
                               *extension.message_info.prototype,
                               extension.descriptor)
                  : MutableMessage(type_id, WireFormatLite::TYPE_MESSAGE,
                                   *extension.message_info.prototype,
                                   extension.descriptor);

          const char* p;
          // We can't use regular parse from string as we have to track
          // proper recursion depth and descriptor pools.
          ParseContext tmp_ctx(ctx->depth(), false, &p, payload);
          tmp_ctx.data().pool = ctx->data().pool;
          tmp_ctx.data().factory = ctx->data().factory;
          GOOGLE_PROTOBUF_PARSER_ASSERT(value->_InternalParse(p, &tmp_ctx) &&
                                         tmp_ctx.EndedAtLimit());
        }
        type_id = 0;
      }
    } else if (tag == WireFormatLite::kMessageSetMessageTag) {
      if (type_id != 0) {
        ptr = ParseFieldMaybeLazily(static_cast<uint64_t>(type_id) * 8 + 2, ptr,
                                    extendee, metadata, ctx);
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr != nullptr);
        type_id = 0;
      } else {
        int32_t size = ReadSize(&ptr);
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        ptr = ctx->ReadString(ptr, size, &payload);
        GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
        payload_read = true;
      }
    } else {
      ptr = ReadTag(ptr - 1, &tag);
      if (tag == 0 || (tag & 7) == 4) {
        ctx->SetLastTag(tag);
        return ptr;
      }
      ptr = ParseField(tag, ptr, extendee, metadata, ctx);
      GOOGLE_PROTOBUF_PARSER_ASSERT(ptr);
    }
  }
  return ptr;
}