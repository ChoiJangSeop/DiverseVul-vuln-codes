tTcpIpPacketParsingResult ParaNdis_CheckSumVerifyFlat(
                                                PVOID pBuffer,
                                                ULONG ulDataLength,
                                                ULONG flags,
                                                LPCSTR caller)
{
    tCompletePhysicalAddress SGBuffer;
    SGBuffer.Virtual = pBuffer;
    SGBuffer.size = ulDataLength;
    return ParaNdis_CheckSumVerify(&SGBuffer, ulDataLength, 0, flags, caller);
}