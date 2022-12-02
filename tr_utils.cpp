#include "tr_thread.h"

// 产生网际校验和
USHORT TRThread::GenerateChecksum(USHORT *pBuf, int iSize) { // 16
    unsigned long cksum = 0;// 32
    while (iSize > 1) { // 40
        cksum += *pBuf++;
        iSize -= sizeof(USHORT);
    }
    if (iSize) {
        cksum += *(UCHAR *) pBuf; // 8
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (USHORT) (~cksum); // ~ 按位取反
}

// 解码得到的数据报，因为涉及到 UI 操作所以放到 TR_GUI 里
BOOL TRThread::DecodeIcmpResponse(char *pBuf, int iPacketSize, DECODE_RESULT &stDecodeResult) {

    // 检查数据报大小的合法性
    IP_HEADER *pIpHdr = (IP_HEADER *) pBuf;
    int iIpHdrLen = pIpHdr->hdr_len * 4;//单位4个字节
    if (iPacketSize < (int) (iIpHdrLen + sizeof(ICMP_HEADER))) {
        return FALSE;
    }

    // 按照ICMP包类型检查id字段和序列号以确定是否是程序应接收的Icmp包
    ICMP_HEADER *pIcmpHdr = (ICMP_HEADER *) (pBuf + iIpHdrLen);
    USHORT usID, usSquNo;//ICMP头 标识符和序列号
    if (pIcmpHdr->type == ICMP_ECHO_REPLY) {
        usID = pIcmpHdr->id;
        usSquNo = pIcmpHdr->seq;
    } else if (pIcmpHdr->type == ICMP_TIMEOUT) {
        char *pInnerIpHdr = pBuf + iIpHdrLen + sizeof(ICMP_HEADER);  //载荷中的IP头
        int iInnerIPHdrLen = ((IP_HEADER *) pInnerIpHdr)->hdr_len * 4;//载荷中的IP头长
        ICMP_HEADER *pInnerIcmpHdr = (ICMP_HEADER *) (pInnerIpHdr + iInnerIPHdrLen);//载荷中的ICMP头
        usID = pInnerIcmpHdr->id;
        usSquNo = pInnerIcmpHdr->seq;
    } else {
        return FALSE;
    }
    if (usID != (USHORT) GetCurrentProcessId() || usSquNo != stDecodeResult.usSeqNo) {
        return FALSE;
    }

    // 处理正确收到的ICMP数据报
    if (pIcmpHdr->type == ICMP_ECHO_REPLY ||
        pIcmpHdr->type == ICMP_TIMEOUT) {

        // 返回解码结果
        stDecodeResult.dwIPaddr.s_addr = pIpHdr->sourceIP;
        return TRUE;
    }
    return FALSE;
}
