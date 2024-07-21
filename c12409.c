RemotePeer *PeerFactory::createPeer(const ProtoList &protocols, AuthHandler *authHandler, QTcpSocket *socket, Compressor::CompressionLevel level, QObject *parent)
{
    foreach(const ProtoDescriptor &protodesc, protocols) {
        Protocol::Type proto = protodesc.first;
        quint16 features = protodesc.second;
        switch(proto) {
            case Protocol::LegacyProtocol:
                return new LegacyPeer(authHandler, socket, level, parent);
            case Protocol::DataStreamProtocol:
                if (DataStreamPeer::acceptsFeatures(features))
                    return new DataStreamPeer(authHandler, socket, features, level, parent);
                break;
            default:
                break;
        }
    }

    return 0;
}