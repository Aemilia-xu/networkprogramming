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
	UCHAR iphVerLen;     //�汾�ź�ͷ���ȣ���4λ��
	UCHAR ipTOS;         //��������
	USHORT ipLength;     //����ܳ��ȣ�������IP���ĳ���
	USHORT ipID;         //�����ʾ��Ψһ��ʶ���͵�ÿһ�����ݱ�
	USHORT ipFLags;      //��־
	UCHAR ipTTL;         // TTL
	UCHAR ipProtocol;    //Э��
	USHORT ipChecksum;   //У���
	ULONG ipSource;      //ԴIP��ַ
	ULONG ipDestination; //Ŀ��IP��ַ
} IPHeader, *PIPHeader;

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
} TCPHeader, *PTCPHeader;

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
	memcpy(&addr_in.sin_addr.S_un.S_addr, pHost->h_addr_list[2], pHost->h_length);
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
			// TODO
			//����Ҫ�������ݰ�
			printf("start sniffing\n");
			return;
			
		}
	}
	closesocket(sRaw);
	::WSACleanup();
}