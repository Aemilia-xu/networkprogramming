#include <stdio.h>
#include <winsock2.h>
#include <Mswsock.h>
#include <mstcpip.h>
#pragma comment(lib, "WS2_32")

#define NONE "\033[m"
#define RED "\033[0;32;31m"
// IP头
typedef struct _IPHeader
{
	UCHAR iphVerLen;     // 版本号和头长度（各4位）
	UCHAR ipTOS;         // 服务类型
	USHORT ipLength;     // 封包总长度，即整个IP报的长度
	USHORT ipID;         // 封包表示，唯一标识发送的每一个数据报
	USHORT ipFLags;      // 标志
	UCHAR ipTTL;         // TTL
	UCHAR ipProtocol;    // 协议
	USHORT ipChecksum;   // 校验和
	ULONG ipSource;      // 源IP地址
	ULONG ipDestination; // 目的IP地址
} IPHeader, * PIPHeader;

// TCP头
typedef struct _TCPHeader
{
	USHORT sourcePort;       // 16位源端口
	USHORT destinationPort;  // 16位目的端口
	ULONG sqeuenceNumber;    // 32位序列号
	ULONG acknowledgeNumber; // 32位确认号
	UCHAR dataoffset;        // 4位首部长度/6位保留字
	UCHAR flags;             // 6位标识位
	USHORT windows;          // 16位窗口大小
	USHORT checksum;         // 16位校验和
	USHORT urgentPointer;    // 16位紧急数据偏移量
} TCPHeader, * PTCPHeader;

// UDP头
typedef struct _UDPHeader
{
	USHORT sourcePort;			// 16位源端口
	USHORT destinationPort;		// 16位目的端口
	USHORT len;					// 16位封包长度
	USHORT checksum;			// 16位校验和
} UDPHeader, * PUDPHeader;

// TCP连接记录
typedef struct _TCPConnection
{
	ULONG ipSource;			// 源IP地址
	ULONG ipDestination;	// 目的IP地址
	USHORT sourcePort;		// 16位源端口
	USHORT destinationPort;	// 16位目的端口
	char count;				// 握手次数
	_TCPConnection* pPrev;	// prev指针
	_TCPConnection* pNext;	// next指针
} connection;

connection connectionHead; // TCP连接链表头
connection connectionTail; // TCP连接链表尾

void addConnection(ULONG ipSource, ULONG ipDestination, USHORT sourcePort, USHORT destinationPort);
void deleteConnection(connection* p);
connection* findConnection(ULONG ipSource, ULONG ipDestination, USHORT sourcePort, USHORT destinationPort);

void DecodeIPPacket(char* pData, int len);
void DecodeTCPPacket(char* pData, int len, ULONG ipSource, ULONG ipDestination);
void DecodeUDPPacket(char* pData, int len);

void main()
{
	// WSAStartup
	BYTE minorVer = 2;
	BYTE majorVer = 2;
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(minorVer, majorVer);
	if (::WSAStartup(sockVersion, &wsaData) != 0)
	{
		exit(0);
	}
	system("cls");
	// 初始化TCP连接链表表头表尾
	connectionHead.pPrev = NULL;
	connectionHead.pNext = (&connectionTail);
	connectionTail.pPrev = (&connectionHead);
	connectionTail.pNext = NULL;
	// 1.创建原始套接字
	SOCKET sRaw = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	// 2.获取本地地址
	char szHostName[56];
	SOCKADDR_IN addr_in;
	struct hostent *pHost;
	memset(szHostName, 0, 56);
	gethostname(szHostName, 56);
	if ((pHost = gethostbyname((char *)szHostName)) == NULL) //获取到了VMnet8的地址
	{
		printf(RED "[+] getHostbyname error!\n" NONE);
		return;
	}
	// 3.将原始套接字绑定到本地地址
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(0);
	memcpy(&addr_in.sin_addr.S_un.S_addr, pHost->h_addr_list[2], pHost->h_length);//修改网卡
	printf("try binding to interface: %s\n", inet_ntoa(addr_in.sin_addr));
	if (::bind(sRaw, (PSOCKADDR)&addr_in, sizeof(addr_in)) == SOCKET_ERROR)
	{
		printf(RED "[+] bind error!\n" NONE);
		return;
	}
	// 4.将sRaw设置为混杂模式
	DWORD dwValue = 1;
	if (::ioctlsocket(sRaw, SIO_RCVALL, &dwValue) != 0) {
		printf(RED "[+] ioctlsocket error!\n" NONE);
		return;
	}
	//5.开始接收数据包
	char buff[1024];
	int nRet;
	while (TRUE) {
		nRet = ::recv(sRaw, buff, 1024, 0);
		if (nRet > 0)
		{
			// 解析数据包
			DecodeIPPacket(buff, nRet);
		}
	}
	closesocket(sRaw);
	::WSACleanup();
}

void DecodeIPPacket(char* pData, int len)
{
	IPHeader* pIPHdr = (IPHeader*)pData;
	in_addr source, dest;
	char szSourceIp[32], szDestIp[32];

	printf("\n\n-------------------------------\n");

	// 从IP头中取出源IP地址和目的IP地址
	source.S_un.S_addr = pIPHdr->ipSource;
	dest.S_un.S_addr = pIPHdr->ipDestination;
	strcpy(szSourceIp, ::inet_ntoa(source));
	strcpy(szDestIp, ::inet_ntoa(dest));

	printf("	%s -> %s \n", szSourceIp, szDestIp);
	// IP头长度
	int nHeaderLen = (pIPHdr->iphVerLen & 0xf) * sizeof(ULONG);

	switch (pIPHdr->ipProtocol)
	{
	case IPPROTO_TCP:
		// TCP协议
		printf("Protocol: TCP\n");
		DecodeTCPPacket(pData + nHeaderLen, len - nHeaderLen, pIPHdr->ipSource, pIPHdr->ipDestination);
		break;
	case IPPROTO_UDP:
		// UDP协议
		printf("Protocol: UDP\n");
		DecodeUDPPacket(pData + nHeaderLen, len - nHeaderLen);
		break;
	case IPPROTO_ICMP:
		printf("Protocol: ICMP\n");
		break;
	}
}

void DecodeUDPPacket(char* pData, int len)
{
	UDPHeader* pUDPHdr = (UDPHeader*)pData;

	printf("Port: %d -> %d \n", ntohs(pUDPHdr->sourcePort), ntohs(pUDPHdr->destinationPort));

	// 输出字节数
	printf("Byte: %d\n", len - 8);

	// 下面还可以根据目的端口号进一步解析应用层协议
	switch (::ntohs(pUDPHdr->destinationPort))
	{
	case 53:
		printf("DNS");
		break;
	case 69:
		printf("TFTP");
		break;
	case 161:
		printf("SNMP");
		break;
	default:
		printf("other udp");
	}
}

void DecodeTCPPacket(char* pData, int len, ULONG ipSource, ULONG ipDestination)
{
	TCPHeader* pTCPHdr = (TCPHeader*)pData;
	connection* p = NULL;

	// 打印端口号
	printf("Port: %d -> %d \n", ntohs(pTCPHdr->sourcePort), ntohs(pTCPHdr->destinationPort));

	const char syn = 0x02;
	const char ack = 0x10;

	switch ((pTCPHdr->flags) & (syn | ack))
	{
	case syn:
		printf(RED "SYN\n" NONE);
		// 如果连接不在链表里，加入链表
		if (findConnection(ipSource, ipDestination, pTCPHdr->sourcePort, pTCPHdr->destinationPort) == NULL)
			addConnection(ipSource, ipDestination, pTCPHdr->sourcePort, pTCPHdr->destinationPort);
		break;
	case (syn | ack):
		printf(RED "SYN+ACK\n" NONE);
		// 如果连接在链表里，表示已经发送过SYN了；设置count为2
		p = findConnection(ipDestination, ipSource, pTCPHdr->destinationPort, pTCPHdr->sourcePort);
		if (p != NULL)
			p->count = 2;
		break;
	case ack:
		// 在非三次握手时，也会有ack出现；如果链表包含连接且已经握手2次，则该ack属于握手过程，进行输出
		p = findConnection(ipSource, ipDestination, pTCPHdr->sourcePort, pTCPHdr->destinationPort);
		if (p != NULL && p->count == 2)
		{
			printf(RED "ACK\n" NONE);
			// 三次握手完成，删除节点
			deleteConnection(p);
		}
		break;
	}

	// 输出字节数
	printf("Byte: %d\n", len - 20);
}

void addConnection(ULONG ipSource, ULONG ipDestination, USHORT sourcePort, USHORT destinationPort) {
	connection* p = (connection*)::GlobalAlloc(GPTR, sizeof(connection));
	if (p != NULL)
	{
		// 设置连接信息
		p->ipSource = ipSource;
		p->ipDestination = ipDestination;
		p->sourcePort = sourcePort;
		p->destinationPort = destinationPort;
		p->count = 1;

		// 插入链表
		connectionTail.pPrev->pNext = p;
		p->pPrev = connectionTail.pPrev;
		p->pNext = (&connectionTail);
		connectionTail.pPrev = p;
	}
	else
	{
		printf("GlobalAlloc Failed.\n");
	}
}

void deleteConnection(connection* p) {
	if (p != NULL)
	{
		// 从链表上摘下节点
		p->pPrev->pNext = p->pNext;
		p->pNext->pPrev = p->pPrev;

		// 释放空间
		::GlobalFree(p);
		p = NULL;
	}
}

connection* findConnection(ULONG ipSource, ULONG ipDestination, USHORT sourcePort, USHORT destinationPort) {
	for (connection* p = connectionHead.pNext; p != (&connectionTail); p = p->pNext)
	{
		if (p->ipSource == ipSource && p->ipDestination == ipDestination && p->sourcePort == sourcePort && p->destinationPort == destinationPort)
			return p;
	}
	return NULL;
}
