#include <stdio.h>
#include <winsock2.h>
#include <Mswsock.h>
#include <mstcpip.h>
#pragma comment(lib, "WS2_32")

#define NONE "\033[m"
#define RED "\033[0;32;31m"
// IPͷ
typedef struct _IPHeader
{
	UCHAR iphVerLen;     // �汾�ź�ͷ���ȣ���4λ��
	UCHAR ipTOS;         // ��������
	USHORT ipLength;     // ����ܳ��ȣ�������IP���ĳ���
	USHORT ipID;         // �����ʾ��Ψһ��ʶ���͵�ÿһ�����ݱ�
	USHORT ipFLags;      // ��־
	UCHAR ipTTL;         // TTL
	UCHAR ipProtocol;    // Э��
	USHORT ipChecksum;   // У���
	ULONG ipSource;      // ԴIP��ַ
	ULONG ipDestination; // Ŀ��IP��ַ
} IPHeader, * PIPHeader;

// TCPͷ
typedef struct _TCPHeader
{
	USHORT sourcePort;       // 16λԴ�˿�
	USHORT destinationPort;  // 16λĿ�Ķ˿�
	ULONG sqeuenceNumber;    // 32λ���к�
	ULONG acknowledgeNumber; // 32λȷ�Ϻ�
	UCHAR dataoffset;        // 4λ�ײ�����/6λ������
	UCHAR flags;             // 6λ��ʶλ
	USHORT windows;          // 16λ���ڴ�С
	USHORT checksum;         // 16λУ���
	USHORT urgentPointer;    // 16λ��������ƫ����
} TCPHeader, * PTCPHeader;

// UDPͷ
typedef struct _UDPHeader
{
	USHORT sourcePort;			// 16λԴ�˿�
	USHORT destinationPort;		// 16λĿ�Ķ˿�
	USHORT len;					// 16λ�������
	USHORT checksum;			// 16λУ���
} UDPHeader, * PUDPHeader;

// TCP���Ӽ�¼
typedef struct _TCPConnection
{
	ULONG ipSource;			// ԴIP��ַ
	ULONG ipDestination;	// Ŀ��IP��ַ
	USHORT sourcePort;		// 16λԴ�˿�
	USHORT destinationPort;	// 16λĿ�Ķ˿�
	char count;				// ���ִ���
	_TCPConnection* pPrev;	// prevָ��
	_TCPConnection* pNext;	// nextָ��
} connection;

connection connectionHead; // TCP��������ͷ
connection connectionTail; // TCP��������β

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
	// ��ʼ��TCP���������ͷ��β
	connectionHead.pPrev = NULL;
	connectionHead.pNext = (&connectionTail);
	connectionTail.pPrev = (&connectionHead);
	connectionTail.pNext = NULL;
	// 1.����ԭʼ�׽���
	SOCKET sRaw = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	// 2.��ȡ���ص�ַ
	char szHostName[56];
	SOCKADDR_IN addr_in;
	struct hostent *pHost;
	memset(szHostName, 0, 56);
	gethostname(szHostName, 56);
	if ((pHost = gethostbyname((char *)szHostName)) == NULL) //��ȡ����VMnet8�ĵ�ַ
	{
		printf(RED "[+] getHostbyname error!\n" NONE);
		return;
	}
	// 3.��ԭʼ�׽��ְ󶨵����ص�ַ
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(0);
	memcpy(&addr_in.sin_addr.S_un.S_addr, pHost->h_addr_list[2], pHost->h_length);//�޸�����
	printf("try binding to interface: %s\n", inet_ntoa(addr_in.sin_addr));
	if (::bind(sRaw, (PSOCKADDR)&addr_in, sizeof(addr_in)) == SOCKET_ERROR)
	{
		printf(RED "[+] bind error!\n" NONE);
		return;
	}
	// 4.��sRaw����Ϊ����ģʽ
	DWORD dwValue = 1;
	if (::ioctlsocket(sRaw, SIO_RCVALL, &dwValue) != 0) {
		printf(RED "[+] ioctlsocket error!\n" NONE);
		return;
	}
	//5.��ʼ�������ݰ�
	char buff[1024];
	int nRet;
	while (TRUE) {
		nRet = ::recv(sRaw, buff, 1024, 0);
		if (nRet > 0)
		{
			// �������ݰ�
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

	// ��IPͷ��ȡ��ԴIP��ַ��Ŀ��IP��ַ
	source.S_un.S_addr = pIPHdr->ipSource;
	dest.S_un.S_addr = pIPHdr->ipDestination;
	strcpy(szSourceIp, ::inet_ntoa(source));
	strcpy(szDestIp, ::inet_ntoa(dest));

	printf("	%s -> %s \n", szSourceIp, szDestIp);
	// IPͷ����
	int nHeaderLen = (pIPHdr->iphVerLen & 0xf) * sizeof(ULONG);

	switch (pIPHdr->ipProtocol)
	{
	case IPPROTO_TCP:
		// TCPЭ��
		printf("Protocol: TCP\n");
		DecodeTCPPacket(pData + nHeaderLen, len - nHeaderLen, pIPHdr->ipSource, pIPHdr->ipDestination);
		break;
	case IPPROTO_UDP:
		// UDPЭ��
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

	// ����ֽ���
	printf("Byte: %d\n", len - 8);

	// ���滹���Ը���Ŀ�Ķ˿ںŽ�һ������Ӧ�ò�Э��
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

	// ��ӡ�˿ں�
	printf("Port: %d -> %d \n", ntohs(pTCPHdr->sourcePort), ntohs(pTCPHdr->destinationPort));

	const char syn = 0x02;
	const char ack = 0x10;

	switch ((pTCPHdr->flags) & (syn | ack))
	{
	case syn:
		printf(RED "SYN\n" NONE);
		// ������Ӳ����������������
		if (findConnection(ipSource, ipDestination, pTCPHdr->sourcePort, pTCPHdr->destinationPort) == NULL)
			addConnection(ipSource, ipDestination, pTCPHdr->sourcePort, pTCPHdr->destinationPort);
		break;
	case (syn | ack):
		printf(RED "SYN+ACK\n" NONE);
		// ����������������ʾ�Ѿ����͹�SYN�ˣ�����countΪ2
		p = findConnection(ipDestination, ipSource, pTCPHdr->destinationPort, pTCPHdr->sourcePort);
		if (p != NULL)
			p->count = 2;
		break;
	case ack:
		// �ڷ���������ʱ��Ҳ����ack���֣������������������Ѿ�����2�Σ����ack�������ֹ��̣��������
		p = findConnection(ipSource, ipDestination, pTCPHdr->sourcePort, pTCPHdr->destinationPort);
		if (p != NULL && p->count == 2)
		{
			printf(RED "ACK\n" NONE);
			// ����������ɣ�ɾ���ڵ�
			deleteConnection(p);
		}
		break;
	}

	// ����ֽ���
	printf("Byte: %d\n", len - 20);
}

void addConnection(ULONG ipSource, ULONG ipDestination, USHORT sourcePort, USHORT destinationPort) {
	connection* p = (connection*)::GlobalAlloc(GPTR, sizeof(connection));
	if (p != NULL)
	{
		// ����������Ϣ
		p->ipSource = ipSource;
		p->ipDestination = ipDestination;
		p->sourcePort = sourcePort;
		p->destinationPort = destinationPort;
		p->count = 1;

		// ��������
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
		// ��������ժ�½ڵ�
		p->pPrev->pNext = p->pNext;
		p->pNext->pPrev = p->pPrev;

		// �ͷſռ�
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
