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
	UCHAR iphVerLen;     //版本号和头长度（各4位）
	UCHAR ipTOS;         //服务类型
	USHORT ipLength;     //封包总长度，即整个IP报的长度
	USHORT ipID;         //封包表示，唯一标识发送的每一个数据报
	USHORT ipFLags;      //标志
	UCHAR ipTTL;         // TTL
	UCHAR ipProtocol;    //协议
	USHORT ipChecksum;   //校验和
	ULONG ipSource;      //源IP地址
	ULONG ipDestination; //目的IP地址
} IPHeader, *PIPHeader;

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
	memcpy(&addr_in.sin_addr.S_un.S_addr, pHost->h_addr_list[2], pHost->h_length);
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
			// TODO
			//这里要解析数据包
			printf("start sniffing\n");
			return;
			
		}
	}
	closesocket(sRaw);
	::WSACleanup();
}