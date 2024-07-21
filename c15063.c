void RtmpProtocol::handle_chunk(RtmpPacket::Ptr packet) {
    auto &chunk_data = *packet;
    switch (chunk_data.type_id) {
        case MSG_ACK: {
            if (chunk_data.buffer.size() < 4) {
                throw std::runtime_error("MSG_ACK: Not enough data");
            }
            //auto bytePeerRecv = load_be32(&chunk_data.buffer[0]);
            //TraceL << "MSG_ACK:" << bytePeerRecv;
            break;
        }

        case MSG_SET_CHUNK: {
            if (chunk_data.buffer.size() < 4) {
                throw std::runtime_error("MSG_SET_CHUNK :Not enough data");
            }
            _chunk_size_in = load_be32(&chunk_data.buffer[0]);
            TraceL << "MSG_SET_CHUNK:" << _chunk_size_in;
            break;
        }

        case MSG_USER_CONTROL: {
            //user control message
            if (chunk_data.buffer.size() < 2) {
                throw std::runtime_error("MSG_USER_CONTROL: Not enough data.");
            }
            uint16_t event_type = load_be16(&chunk_data.buffer[0]);
            chunk_data.buffer.erase(0, 2);
            switch (event_type) {
                case CONTROL_PING_REQUEST: {
                    if (chunk_data.buffer.size() < 4) {
                        throw std::runtime_error("CONTROL_PING_REQUEST: Not enough data.");
                    }
                    uint32_t timeStamp = load_be32(&chunk_data.buffer[0]);
                    //TraceL << "CONTROL_PING_REQUEST:" << time_stamp;
                    sendUserControl(CONTROL_PING_RESPONSE, timeStamp);
                    break;
                }

                case CONTROL_PING_RESPONSE: {
                    if (chunk_data.buffer.size() < 4) {
                        throw std::runtime_error("CONTROL_PING_RESPONSE: Not enough data.");
                    }
                    //uint32_t time_stamp = load_be32(&chunk_data.buffer[0]);
                    //TraceL << "CONTROL_PING_RESPONSE:" << time_stamp;
                    break;
                }

                case CONTROL_STREAM_BEGIN: {
                    //开始播放
                    if (chunk_data.buffer.size() < 4) {
                        WarnL << "CONTROL_STREAM_BEGIN: Not enough data:" << chunk_data.buffer.size();
                        break;
                    }
                    uint32_t stream_index = load_be32(&chunk_data.buffer[0]);
                    onStreamBegin(stream_index);
                    TraceL << "CONTROL_STREAM_BEGIN:" << stream_index;
                    break;
                }

                case CONTROL_STREAM_EOF: {
                    //暂停
                    if (chunk_data.buffer.size() < 4) {
                        throw std::runtime_error("CONTROL_STREAM_EOF: Not enough data.");
                    }
                    uint32_t stream_index = load_be32(&chunk_data.buffer[0]);
                    onStreamEof(stream_index);
                    TraceL << "CONTROL_STREAM_EOF:" << stream_index;
                    break;
                }

                case CONTROL_STREAM_DRY: {
                    //停止播放
                    if (chunk_data.buffer.size() < 4) {
                        throw std::runtime_error("CONTROL_STREAM_DRY: Not enough data.");
                    }
                    uint32_t stream_index = load_be32(&chunk_data.buffer[0]);
                    onStreamDry(stream_index);
                    TraceL << "CONTROL_STREAM_DRY:" << stream_index;
                    break;
                }

                default: /*WarnL << "unhandled user control:" << event_type; */ break;
            }
            break;
        }

        case MSG_WIN_SIZE: {
            _windows_size = load_be32(&chunk_data.buffer[0]);
            TraceL << "MSG_WIN_SIZE:" << _windows_size;
            break;
        }

        case MSG_SET_PEER_BW: {
            _bandwidth = load_be32(&chunk_data.buffer[0]);
            _band_limit_type =  chunk_data.buffer[4];
            TraceL << "MSG_SET_PEER_BW:" << _windows_size;
            break;
        }

        case MSG_AGGREGATE: {
            auto ptr = (uint8_t *) chunk_data.buffer.data();
            auto ptr_tail = ptr + chunk_data.buffer.size();
            uint32_t latest_ts, timestamp;
            timestamp = chunk_data.time_stamp;
            bool first_message = true;
            while (ptr + 8 + 3 < ptr_tail) {
                auto type = *ptr;
                ptr += 1;
                auto size = load_be24(ptr);
                ptr += 3;
                auto ts = load_be24(ptr);
                ptr += 3;
                ts |= (*ptr << 24);
                ptr += 1;
                ptr += 3;
                //参考FFmpeg多拷贝了4个字节
                size += 4;
                if (ptr + size > ptr_tail) {
                    break;
                }
                if (!first_message) {
                    timestamp += ts - latest_ts;
                }
                first_message = false;
                latest_ts = ts;
                auto sub_packet_ptr = RtmpPacket::create();
                auto &sub_packet = *sub_packet_ptr;
                sub_packet.buffer.assign((char *)ptr, size);
                sub_packet.type_id = type;
                sub_packet.body_size = size;
                sub_packet.time_stamp = timestamp;
                sub_packet.stream_index = chunk_data.stream_index;
                sub_packet.chunk_id = chunk_data.chunk_id;
                handle_chunk(std::move(sub_packet_ptr));
                ptr += size;
            }
            break;
        }

        default: onRtmpChunk(std::move(packet)); break;
    }
}