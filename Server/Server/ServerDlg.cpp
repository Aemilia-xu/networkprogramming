
// ServerDlg.cpp: 实现文件
//


//#pragma comment(lib, "WS2_32")	// 链接到WS2_32.lib

#include "pch.h"
#include "framework.h"
#include "Server.h"
#include "ServerDlg.h"
#include "afxdialogex.h"
#include <string>


#ifdef _DEBUG
#define new DEBUG_NEW
#define BUFFER_SIZE 1024
#endif

// 自定义消息id
#define WM_MY_MESSAGE (WM_USER+100)
#define WM_MY_THREADNUM (WM_USER+101)
#define NEW_SOCKET (WM_USER+102)
#define WM_MY_FILE (WM_USER+103)
#define WSA_MAXIMUM_WAIT_EVENTS 2
// 事件句柄数组和链表地址
//HANDLE g_events[WSA_MAXIMUM_WAIT_EVENTS];	// I/O事件句柄数组
//int g_nBufferCount;							// 上数组中有效句柄数量
//PBUFFER_OBJ g_pBufferHead, g_pBufferTail;	// 记录缓冲区对象组成的表的地址

int PthreadNum=0;                            //记录线程数量
PTHREAD_OBJ g_pThreadList;                 //指向线程对象列表表头
PSOCKET_OBJ g_pSocketList;		           // 指向socket对象列表表头，方便打印现在的socket链接信息
CRITICAL_SECTION g_cs;                     //同步对此全局变量的访问
LONG g_nTatolConnections;                  //总共连接数量
LONG g_nCurrentConnections;                 //当前连接数量

//SeverThreadk
class ServerThreadParam
{	public:
		int test;
		HWND hWnd;
		PTHREAD_OBJ pThread;
};


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CServerDlg 对话框



CServerDlg::CServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST2, threadNum);
	DDX_Control(pDX, IDC_LIST3, connectionList);
	DDX_Control(pDX, IDC_LIST7, clientList);
	DDX_Control(pDX, IDC_LIST5, threadInfo);
	DDX_Control(pDX, IDC_LIST6, dialog);
	DDX_Control(pDX, IDC_LIST8, fileStatus);
}

BEGIN_MESSAGE_MAP(CServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	// 映射消息处理函数
	ON_MESSAGE(WM_MY_MESSAGE, &CServerDlg::OnMyMessage)
	ON_MESSAGE(WM_MY_THREADNUM, &CServerDlg::PrintThread)
	ON_MESSAGE(NEW_SOCKET, &CServerDlg::PrintSocket)
	ON_MESSAGE(WM_MY_FILE, &CServerDlg::OnMyFile)
END_MESSAGE_MAP()


// CServerDlg 消息处理程序

BOOL CServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// 在此添加额外的初始化代码
	// WSAStartup
	BYTE minorVer = 2;
	BYTE majorVer = 2;
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(minorVer, majorVer);
	if (::WSAStartup(sockVersion, &wsaData) != 0)
	{
		exit(0);
	}

	HWND hWnd = AfxGetMainWnd()->m_hWnd;
	//将RunServer作为子线程运行，进行监听
	CWinThread* RecvThread = AfxBeginThread(RunServer, (LPVOID)&hWnd);
	Sleep(1000);// 增加sleep后可保证每次运行正常显示线程
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR CServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 申请套接字对象
//将重叠I/O和线程池相结合
PSOCKET_OBJ CServerDlg::GetSocketObj(SOCKET s)
{
	PSOCKET_OBJ pSocket = (PSOCKET_OBJ)::GlobalAlloc(GPTR, sizeof(SOCKET_OBJ));
	if (pSocket != NULL)
	{
		pSocket->s = s;
	}
	::InitializeCriticalSection(&pSocket->s_cs);
	return pSocket;
}


//将套接字加入到列表中
void CServerDlg::AddSocketOBJ(HWND hWnd,PSOCKET_OBJ pSocket)
{
	if (pSocket != NULL)
	{
		::EnterCriticalSection(&g_cs);
		pSocket->pNext = g_pSocketList;
		g_pSocketList = pSocket;
		::LeaveCriticalSection(&g_cs);
	}
}



// 释放套接字对象
void CServerDlg::FreeSocketObj(HWND hWnd,PSOCKET_OBJ pSocket)
{
	::EnterCriticalSection(&pSocket->s_cs);
	::EnterCriticalSection(&g_cs);
	PSOCKET_OBJ p = g_pSocketList;
	if (p == pSocket)
	{
		g_pSocketList = p->pNext;
	}
	else
	{
		while (p != NULL && p->pNext != pSocket) 
		{
			p = p->pNext;
		}
		if (p != NULL)
		{
			p->pNext = pSocket->pNext;
		}
	}
	::LeaveCriticalSection(&g_cs);
	if (pSocket->s != INVALID_SOCKET)
		::closesocket(pSocket->s);
	char message[128];
	memset(message, 0, 128);
	sprintf_s(message, "[释放连接]%s %d", inet_ntoa(pSocket->addrRemote.sin_addr), pSocket->addrRemote.sin_port);
	::LeaveCriticalSection(&pSocket->s_cs);
	::SendMessage(hWnd, NEW_SOCKET, (WPARAM)message, 0);
	::DeleteCriticalSection(&pSocket->s_cs);
	::GlobalFree(pSocket);

}

// 申请缓冲区对象
PBUFFER_OBJ CServerDlg::GetBufferObj(HWND hWnd,PSOCKET_OBJ pSocket, ULONG nLen)
{
	PBUFFER_OBJ pBuffer = (PBUFFER_OBJ)::GlobalAlloc(GPTR, sizeof(BUFFER_OBJ));
	if (pBuffer != NULL)
	{
		pBuffer->buff = (char*)::GlobalAlloc(GPTR, nLen);
		pBuffer->ol.hEvent = ::WSACreateEvent();
		pBuffer->pSocket = pSocket;
		pBuffer->sAccept = INVALID_SOCKET;

		// 将新的BUFFER_OBJ分配给空闲线程
		AssignToFreeThread(hWnd,pBuffer);
	}
	::InitializeCriticalSection(&pBuffer->s_cs);
	return pBuffer;
}

// 释放缓冲区对象
void CServerDlg::FreeBufferObj(PTHREAD_OBJ pThread, PBUFFER_OBJ pBuffer)
{
	// 从列表中移除BUFFER_OBJ对象
	::EnterCriticalSection(&pThread->cs);
	PBUFFER_OBJ pTest = pThread->pBufferHead;
	BOOL bFind = FALSE;
	if (pTest == pBuffer)
	{
		if (pThread->pBufferHead == pThread->pBufferTail)
			pThread->pBufferHead = pThread->pBufferTail = NULL;
		else pThread->pBufferHead = pTest->pNext;
		pThread->nBufferCount--;
		bFind = TRUE;
	}
	else
	{
		while (pTest != NULL && pTest->pNext != pBuffer)
			pTest = pTest->pNext;
		if (pTest != NULL)
		{
			pTest->pNext = pBuffer->pNext;
			if (pTest->pNext == NULL)
				pThread->pBufferTail = pTest;
			pThread->nBufferCount--;
			bFind = TRUE;
		}
	}
	::LeaveCriticalSection(&pThread->cs);

	// 指示线程重建句柄数组
	::WSASetEvent(pThread->events[0]);
	// 释放它占用的内存空间
	if (bFind)
	{
		::CloseHandle(pBuffer->ol.hEvent);
		::DeleteCriticalSection(&pBuffer->s_cs);
		::GlobalFree(pBuffer->buff);
		::GlobalFree(pBuffer);
	}
}

// 根据事件查找相应的缓冲区对象
PBUFFER_OBJ CServerDlg::FindBufferObj(PTHREAD_OBJ pThread, int nIndex)
{
	PBUFFER_OBJ pBuffer = pThread->pBufferHead;
	while (--nIndex)
	{
		if (pBuffer == NULL)
			return NULL;
		pBuffer = pBuffer->pNext;
	}
	return pBuffer;
}

// 提交接受连接的缓冲区对象
BOOL CServerDlg::PostAccept(PBUFFER_OBJ pBuffer)
{
	PSOCKET_OBJ pSocket = pBuffer->pSocket;
	if (pSocket->lpfnAcceptEx != NULL)
	{
		// 设置I/O类型，增加套节字上的重叠I/O计数
		pBuffer->nOperation = OP_ACCEPT;
		::EnterCriticalSection(&pSocket->s_cs);
		pSocket->nOutstandingOps++;
		::LeaveCriticalSection(&pSocket->s_cs);
		// 投递此重叠I/O  
		DWORD dwBytes;
		pBuffer->sAccept =
			::WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		BOOL b = pSocket->lpfnAcceptEx(pSocket->s,
			pBuffer->sAccept,  
			pBuffer->buff,
			BUFFER_SIZE - ((sizeof(sockaddr_in) + 16) * 2),
			sizeof(sockaddr_in) + 16,
			sizeof(sockaddr_in) + 16,
			&dwBytes,
			&pBuffer->ol);
		if (!b)
		{
			if (::WSAGetLastError() != WSA_IO_PENDING)
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;
};

// 提交接收数据的缓冲区对象
BOOL CServerDlg::PostRecv(PSOCKET_OBJ pSocket,PBUFFER_OBJ pBuffer)
{
	// 设置I/O类型，增加套节字上的重叠I/O计数
	pBuffer->nOperation = OP_READ;
	::EnterCriticalSection(&pBuffer->pSocket->s_cs);
	pBuffer->pSocket->nOutstandingOps++;
	::LeaveCriticalSection(&pBuffer->pSocket->s_cs);
	// 投递此重叠I/O
	DWORD dwBytes;
	DWORD dwFlags = 0;
	WSABUF buf;
	buf.buf = pBuffer->buff;
	buf.len = pBuffer->nLen;
	if (::WSARecv(pBuffer->pSocket->s, &buf, 1, &dwBytes, &dwFlags, &pBuffer->ol, NULL) != NO_ERROR)
	{
		if (::WSAGetLastError() != WSA_IO_PENDING)
			return FALSE;
	}

	return TRUE;
}

// 提交发送数据的缓冲区对象
BOOL CServerDlg::PostSend(PBUFFER_OBJ pBuffer)
{
	// 设置I/O类型，增加套节字上的重叠I/O计数
	pBuffer->nOperation = OP_WRITE;
	::EnterCriticalSection(&pBuffer->pSocket->s_cs);
	pBuffer->pSocket->nOutstandingOps++;
	::LeaveCriticalSection(&pBuffer->pSocket->s_cs);

	// 如果与要读的下一个序列号相等，则读这块缓冲区
	if (pBuffer->nSequenceNumber == pBuffer->pSocket->nCurrentReadSequence) {	// socket当前想读的序列号是缓存区对象的序列号
		// 投递此重叠I/O
		DWORD dwBytes;
		DWORD dwFlags = 0;
		WSABUF buf;
		buf.buf = pBuffer->buff;
		buf.len = pBuffer->nLen;
		if (::WSASend(pBuffer->pSocket->s,
			&buf, 1, &dwBytes, dwFlags, &pBuffer->ol, NULL) != NO_ERROR)
		{
			if (::WSAGetLastError() != WSA_IO_PENDING)
				return FALSE;
		}
		return TRUE;
	}
	// 如果不相等，则说明没有按顺序接收数据，将这块缓冲区保存到连接的pOutOfOrderReads列表中
	// 列表中的缓冲区是按照其序列号从小到大的顺序排列的
	else {
		PBUFFER_OBJ ptr = pBuffer->pSocket->pOutOfOrderReads;
		PBUFFER_OBJ pPre = NULL;
		while (ptr != NULL){
			if (pBuffer->nSequenceNumber < ptr->nSequenceNumber)	break;
			pPre = ptr;
			ptr = ptr->pNext;
		}
		if (pPre == NULL) { // 应该插入到表头
			pBuffer->pNext = pBuffer->pSocket->pOutOfOrderReads;
			pBuffer->pSocket->pOutOfOrderReads = pBuffer;
		}
		else {// 应该插入到表的中间
			pBuffer->pNext = pPre->pNext;
			pPre->pNext = pBuffer->pNext;
		}

		// 检查表头元素的序列号，如果与要读的序列号一致，就将它从表中移除，返回给用户
		ptr = pBuffer->pSocket->pOutOfOrderReads;
		if (ptr != NULL && (ptr->nSequenceNumber == pBuffer->pSocket->nCurrentReadSequence)){
			pBuffer->pSocket->pOutOfOrderReads = ptr->pNext;
			PostSend(ptr);
		}
		return NULL;
	}
}

// 发送文件
BOOL CServerDlg::SendFile(PBUFFER_OBJ pBuffer, HWND hWnd)
{
	//发送“传输开始”
	char* start = "传输开始";
	pBuffer->buff = start;
	pBuffer->nLen = int(strlen(start));

	::EnterCriticalSection(&pBuffer->pSocket->s_cs);
	pBuffer->pSocket->nCurrentReadSequence++;
	pBuffer->pSocket->nReadSequence++;//下一个序列号
	::LeaveCriticalSection(&pBuffer->pSocket->s_cs);
	::EnterCriticalSection(&pBuffer->s_cs);
	pBuffer->nSequenceNumber++;
	::LeaveCriticalSection(&pBuffer->s_cs);

	PostSend(pBuffer);

	// 创建文件句柄
	CString filename = _T("./res/send.gif");
	HANDLE hFile;
	hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	unsigned long long file_size = GetFileSize(hFile, NULL);
	char FileLen[30] = { 0 };	//ull转char*
	sprintf_s(FileLen, 30, "%llu", file_size);
	// 发送文件大小
	pBuffer->buff = FileLen;
	pBuffer->nLen = int(strlen(FileLen));

	::EnterCriticalSection(&pBuffer->pSocket->s_cs);
	pBuffer->pSocket->nCurrentReadSequence++;
	pBuffer->pSocket->nReadSequence++;//下一个序列号
	::LeaveCriticalSection(&pBuffer->pSocket->s_cs);
	::EnterCriticalSection(&pBuffer->s_cs);
	pBuffer->nSequenceNumber++;
	::LeaveCriticalSection(&pBuffer->s_cs);

	PostSend(pBuffer);
	::PostMessage(hWnd, WM_MY_FILE, (WPARAM)("传输文件大小："), 0);
	::PostMessage(hWnd, WM_MY_FILE, (WPARAM)(FileLen), 0);
	// 读取文件
	char Buffer[1024];
	DWORD dwNumberOfBytesRead;
	do
	{
		::ReadFile(hFile, Buffer, sizeof(Buffer), &dwNumberOfBytesRead, NULL);
		pBuffer->buff = Buffer;
		pBuffer->nLen = sizeof(Buffer);

		::EnterCriticalSection(&pBuffer->pSocket->s_cs);
		pBuffer->pSocket->nCurrentReadSequence++;
		pBuffer->pSocket->nReadSequence++;//下一个序列号
		::LeaveCriticalSection(&pBuffer->pSocket->s_cs);
		::EnterCriticalSection(&pBuffer->s_cs);
		pBuffer->nSequenceNumber++;
		::LeaveCriticalSection(&pBuffer->s_cs);
		PostSend(pBuffer);
	} while (dwNumberOfBytesRead);
	CloseHandle(hFile);

	char* end = "传输结束";
	pBuffer->buff = end;
	pBuffer->nLen = int(strlen(end));

	::EnterCriticalSection(&pBuffer->pSocket->s_cs);
	pBuffer->pSocket->nCurrentReadSequence++;
	pBuffer->pSocket->nReadSequence++;//下一个序列号
	::LeaveCriticalSection(&pBuffer->pSocket->s_cs);
	::EnterCriticalSection(&pBuffer->s_cs);
	pBuffer->nSequenceNumber++;
	::LeaveCriticalSection(&pBuffer->s_cs);
	PostSend(pBuffer);
	return true;
}


BOOL CServerDlg::HandleIO(PTHREAD_OBJ pThread, PBUFFER_OBJ pBuffer, HWND hWnd)
{
	PSOCKET_OBJ pSocket = pBuffer->pSocket; // 从BUFFER_OBJ对象中提取SOCKET_OBJ对象指针，为的是方便引用
	::EnterCriticalSection(&pSocket->s_cs);
	pSocket->nOutstandingOps--;
	::LeaveCriticalSection(&pSocket->s_cs);
	// 获取重叠操作结果
	DWORD dwTrans;
	DWORD dwFlags;
	BOOL bRet = ::WSAGetOverlappedResult(pSocket->s, &pBuffer->ol, &dwTrans, FALSE, &dwFlags);
	if (!bRet)
	{
		// 在此套节字上有错误发生，因此，关闭套节字，移除此缓冲区对象。
		// 如果没有其它抛出的I/O请求了，释放此缓冲区对象，否则，等待此套节字上的其它I/O也完成
		::EnterCriticalSection(&pSocket->s_cs);
		if (pSocket->s != INVALID_SOCKET)
		{
			::closesocket(pSocket->s);
			pSocket->s = INVALID_SOCKET;
		}
		::LeaveCriticalSection(&pSocket->s_cs);
		if (pSocket->nOutstandingOps == 0)
			FreeSocketObj(hWnd,pSocket);

		FreeBufferObj(pThread, pBuffer);
		return FALSE;
	}

	// 没有错误发生，处理已完成的I/O
	switch (pBuffer->nOperation)
	{
	case OP_ACCEPT:	// 接收到一个新的连接，并接收到了对方发来的第一个封包
	{
		// 为新客户创建一个SOCKET_OBJ对象
		PSOCKET_OBJ pClient = GetSocketObj(pBuffer->sAccept);
		AddSocketOBJ(hWnd,pClient);
		// 为发送数据创建一个BUFFER_OBJ对象，这个对象会在套节字出错或者关闭时释放
		PBUFFER_OBJ pSend = GetBufferObj(hWnd, pClient, BUFFER_SIZE);
		if (pSend == NULL)
		{
			//printf(" Too much connections! \n");  
			::SendMessage(hWnd, WM_MY_MESSAGE, (WPARAM)"Too much connections!\n", 0);
			FreeSocketObj(hWnd,pClient);
			return FALSE;
		}
		RebuildArray(pThread);

		//读取socket四元组
		int nLocalLen, nRmoteLen;
		LPSOCKADDR pLocalAddr, pRemoteAddr;
		pSocket->lpfnGetAcceptExSockaddrs(
			pBuffer->buff,
			BUFFER_SIZE - ((sizeof(sockaddr_in) + 16) * 2),
			sizeof(sockaddr_in) + 16,
			sizeof(sockaddr_in) + 16,
			(SOCKADDR**)&pLocalAddr,
			&nLocalLen,
			(SOCKADDR**)&pRemoteAddr,
			&nRmoteLen);
		memcpy(&pClient->addrLocal, pLocalAddr, nLocalLen);
		memcpy(&pClient->addrRemote, pRemoteAddr, nRmoteLen);
		char message[128];
		memset(message, 0, 128);
		sprintf_s(message, "[创建连接]%s %d", inet_ntoa(pClient->addrRemote.sin_addr), pClient->addrRemote.sin_port);
		::SendMessage(hWnd, NEW_SOCKET, (WPARAM)message, 0);

		// 将数据复制到发送缓冲区
		pSend->nLen = dwTrans;
		memcpy(pSend->buff, pBuffer->buff, dwTrans);

		::EnterCriticalSection(&pBuffer->pSocket->s_cs);
		pSend->pSocket->nCurrentReadSequence = 0;
		pSend->pSocket->nReadSequence = pSend->pSocket->nCurrentReadSequence + 1;//下一个序列号
		::LeaveCriticalSection(&pBuffer->pSocket->s_cs);
		::EnterCriticalSection(&pBuffer->s_cs);
		pSend->nSequenceNumber = 0;
		::LeaveCriticalSection(&pBuffer->s_cs);

		// 投递此发送I/O（将数据回显给客户）
		if (!PostSend(pSend))
		{
			// 万一出错的话，释放上面刚申请的两个对象
			FreeSocketObj(hWnd,pSocket);
			FreeBufferObj(pThread, pSend);
			return FALSE;
		}
		::SendMessage(hWnd, WM_MY_MESSAGE, (WPARAM)pBuffer->buff, 0);

		// 继续投递接受I/O
		PostAccept(pBuffer);
	}
	break;
	case OP_READ:	// 接收数据完成
	{
		if (dwTrans > 0)
		{
			// 创建一个缓冲区，以发送数据。这里就使用原来的缓冲区
			PBUFFER_OBJ pSend = pBuffer;
			pSend->nLen = dwTrans;
			const char* flag = "请求发送文件";

			if (strcmp(pSend->buff, flag) != 0) {
				// 字符串不相等，说明发送的是message，投递发送I/O（将数据回显给客户）
				::EnterCriticalSection(&pBuffer->pSocket->s_cs);
				pBuffer->pSocket->nCurrentReadSequence++;
				pBuffer->pSocket->nReadSequence++;//下一个序列号
				::LeaveCriticalSection(&pBuffer->pSocket->s_cs);
				::EnterCriticalSection(&pBuffer->s_cs);
				pBuffer->nSequenceNumber++;
				::LeaveCriticalSection(&pBuffer->s_cs);
				PostSend(pSend);
				::SendMessage(hWnd, WM_MY_MESSAGE, (WPARAM)pBuffer->buff, 0);
			}
			else {// 发送文件
				::PostMessage(hWnd, WM_MY_FILE, (WPARAM)("传输开始"), 0);
				SendFile(pSend, hWnd);
				Sleep(500);
				::PostMessage(hWnd, WM_MY_FILE, (WPARAM)("传输结束"), 0);
			}
		}
		else	// 套节字关闭
		{

			// 必须先关闭套节字，以便在此套节字上投递的其它I/O也返回
			::EnterCriticalSection(&pSocket->s_cs);
			if (pSocket->s != INVALID_SOCKET)
			{
				::closesocket(pSocket->s);
				pSocket->s = INVALID_SOCKET;
			}
			::LeaveCriticalSection(&pSocket->s_cs);

			if (pSocket->nOutstandingOps == 0)
				FreeSocketObj(hWnd,pSocket);

			FreeBufferObj(pThread, pBuffer);
			return FALSE;
		}
	}
	break;
	case OP_WRITE:		// 发送数据完成
	{
		if (dwTrans > 0)
		{
			//TODO:这里是要继续使用呢，还是直接释放呢？
			// 继续使用这个缓冲区投递接收数据的请求
			pBuffer->nLen = BUFFER_SIZE;
			PostRecv(pSocket,pBuffer);
		}
		else	// 套节字关闭
		{
			// 同样，要先关闭套节字
			::EnterCriticalSection(&pSocket->s_cs);
			if (pSocket->s != INVALID_SOCKET)
			{
				::closesocket(pSocket->s);
				pSocket->s = INVALID_SOCKET;
			}
			::LeaveCriticalSection(&pSocket->s_cs);
			if (pSocket->nOutstandingOps == 0)
				FreeSocketObj(hWnd,pSocket);

			FreeBufferObj(pThread, pBuffer);
			return FALSE;
		}
	}
	break;
	}
	return TRUE;
}

UINT CServerDlg::RunServer(LPVOID lpVoid) {
	// 获取参数
	HWND hWnd = *((HWND *)lpVoid);

	// 创建监听套节字，绑定到本地端口，进入监听模式
	int nPort = 4567;
	SOCKET sListen = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN si;
	si.sin_family = AF_INET;
	si.sin_port = ::ntohs(nPort);
	si.sin_addr.S_un.S_addr = INADDR_ANY;
	if (::bind(sListen, (sockaddr*)&si, sizeof(si)) == SOCKET_ERROR) {
		::SendMessage(hWnd, WM_MY_MESSAGE, (WPARAM)"Failed bind()\n", 0);
		int error = WSAGetLastError();
		return 0;
	}
	if (::listen(sListen, 200) == SOCKET_ERROR) {
		::SendMessage(hWnd, WM_MY_MESSAGE, (WPARAM)"Failed listen()\n", 0);
		int error = WSAGetLastError();
		return 0;
	}

	// 为监听套节字创建一个SOCKET_OBJ对象
	PSOCKET_OBJ pListen = GetSocketObj(sListen);

	// 加载扩展函数AcceptEx
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;
	WSAIoctl(pListen->s,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx,
		sizeof(GuidAcceptEx),
		&pListen->lpfnAcceptEx,
		sizeof(pListen->lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL);
	
	// 加载扩展函数GetAcceptExSockaddrs
	GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
	::WSAIoctl(pListen->s,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAcceptExSockaddrs,
		sizeof(GuidGetAcceptExSockaddrs),
		&pListen->lpfnGetAcceptExSockaddrs,
		sizeof(pListen->lpfnGetAcceptExSockaddrs),
		&dwBytes,
		NULL,
		NULL
	);
	::InitializeCriticalSection(&g_cs);
	// 在此可以投递多个接受I/O请求

	for (int i = 0; i < 5; i++)
	{
		PostAccept(GetBufferObj(hWnd, pListen, BUFFER_SIZE)); //GetBuffer中将buffer_obj分配给空闲线程
		Sleep(10000);
		
	}
	while (TRUE);
	::DeleteCriticalSection(&g_cs);
	
}

// 消息处理函数
LRESULT CServerDlg::OnMyMessage(WPARAM wParam, LPARAM lParam) {
	// 类型转换
	::EnterCriticalSection(&g_cs);
	CString str;
	str = (char*)wParam;

	// 显示
	int count = dialog.GetCount();//获得行数，在之后加上发送的内容
	dialog.InsertString(count, str);
	str.Format(_T("%d"), PthreadNum);
	threadNum.AddString(str);
	::LeaveCriticalSection(&g_cs);
	return 0;
}

// fileStatus消息处理函数
LRESULT CServerDlg::OnMyFile(WPARAM wParam, LPARAM lParam) {
	// 类型转换
	CString str;
	str = (char*)wParam;

	// 显示
	int count = fileStatus.GetCount();//获得行数，在之后加上发送的内容
	fileStatus.InsertString(count, str);
	return 0;
}

//打印当前线程状态
LRESULT CServerDlg::PrintThread(WPARAM wParam, LPARAM lParam) 
{	
	::EnterCriticalSection(&g_cs);
	CString str;
	int count = 0,line=0;
	line = threadInfo.GetCount();
	str= (char*)wParam;
	threadInfo.InsertString(line, str); line++;
	threadNum.ResetContent();
	str.Format(_T("%d"), PthreadNum);
	threadNum.AddString(str);

	
	str.Format(_T("线程状态:"));
	threadInfo.InsertString(line, str); line++;
	//threadNum.AddString(str);

	PTHREAD_OBJ pThread = g_pThreadList;
	while (pThread != NULL)
	{
		count++;
		str.Format(_T("thread #%d nBufferCount=%d"), count, pThread->nBufferCount);
		threadInfo.InsertString(line, str); line++;
		pThread = pThread->pNext;
	}
	
	::LeaveCriticalSection(&g_cs);
	return 0;
}

//打印当前Socket状态
LRESULT CServerDlg::PrintSocket(WPARAM wParam, LPARAM lParam)
{
	::EnterCriticalSection(&g_cs);
	CString str;
	int line, count = 0;;
	str = (char*)wParam;//这里打印请求连接或者关闭连接之类的信息（要包含客户的ip地址、端口等，最好可以加个时间？）
	line = dialog.GetCount();
	dialog.InsertString(line, str);
	line = 0;
	//打印当前服务器接受的连接
	connectionList.ResetContent();
	PSOCKET_OBJ pSocket = g_pSocketList;
	while (pSocket != NULL)
	{
		count++;
		char message[128];
		memset(message, 0, 128);
		sprintf_s(message, "connection #%d from %s %d\n", count,inet_ntoa(pSocket->addrRemote.sin_addr), pSocket->addrRemote.sin_port);
		str = message;
		connectionList.InsertString(line, str); line++;
		pSocket = pSocket->pNext;
	}
	::LeaveCriticalSection(&g_cs);
	return 0;
}



//申请线程对象，初始化其成员，并将它添加到线程对象列表中
PTHREAD_OBJ CServerDlg::GetThreadObj(HWND hWnd)
{
	PTHREAD_OBJ pThread = (PTHREAD_OBJ)::GlobalAlloc(GPTR, sizeof(THREAD_OBJ));
	if (pThread != NULL)
	{
		::InitializeCriticalSection(&pThread->cs);
		//创建一个事件对象，用于指示该线程的句柄数组需要重建
		pThread->events[0] = ::WSACreateEvent();
		
		//将新申请的线程对象添加到列表中
		::EnterCriticalSection(&g_cs);
		pThread->pNext = g_pThreadList;
		g_pThreadList = pThread;
		PthreadNum++;    //更新线程数量
		::LeaveCriticalSection(&g_cs);
		//::SendMessage(hWnd, WM_MY_THREADNUM, (WPARAM)"创建了新线程\n", 0);
		
	}
	//更新线程数量
	return pThread;
}

//释放一个线程对象，并将它从线程对象列表中移除
void CServerDlg::FreeThreadObj(HWND hWnd, PTHREAD_OBJ pThread) {

	//在线程对象列表中查找pThread所指的对象，如果找到就从中移除
	::EnterCriticalSection(&g_cs);
	PTHREAD_OBJ p = g_pThreadList;
	if (p == pThread) {   //检查是否是线程列表中第一个
		g_pThreadList = p->pNext;
	}
	else {
		while (p != NULL && p->pNext != pThread) {
			p = p->pNext;
		}
		if (p != NULL) { //此时p->pNext==pThread
			p->pNext = pThread->pNext;
		}
	}
	//更新线程数量
	PthreadNum--;    //更新线程数量
	::LeaveCriticalSection(&g_cs);

	//接下来释放资源
	::CloseHandle(pThread->events[0]);
	::DeleteCriticalSection(&pThread->cs);
	::GlobalFree(pThread);

}


//重新建立线程对象的events数组
void CServerDlg::RebuildArray(PTHREAD_OBJ pThread) 
{
	::EnterCriticalSection(&pThread->cs);
	PBUFFER_OBJ pBuffer = pThread->pBufferHead;
	int n = 1;    
	while (pBuffer != NULL) 
	{
		pThread->events[n++] = pBuffer->ol.hEvent;
		pBuffer = pBuffer->pNext;

	}
	::LeaveCriticalSection(&pThread->cs);

}

//向一个线程的缓冲区列表中插入一个缓冲区
BOOL CServerDlg::InsertBufferObj(PTHREAD_OBJ pThread, PBUFFER_OBJ pBuffer)
{
	BOOL bRet = FALSE;
	::EnterCriticalSection(&pThread->cs);
	if (pThread->nBufferCount < WSA_MAXIMUM_WAIT_EVENTS - 1)
	{
		if (pThread->pBufferHead == NULL) 
		{
			pThread->pBufferTail = pThread->pBufferHead= pBuffer;
		}
		else
		{
			pThread->pBufferTail->pNext = pBuffer;
			pThread->pBufferTail = pBuffer;
		}
		pThread->nBufferCount++;
		bRet = TRUE;
	}
	::LeaveCriticalSection(&pThread->cs);
	//插入成功，说明成功处理了客户的连接请求
	if (bRet) {
		::InterlockedIncrement(&g_nTatolConnections);
		::InterlockedIncrement(&g_nCurrentConnections);
	}
	return bRet;
}


//将一个缓冲区对象安排给空闲的线程处理
void CServerDlg::AssignToFreeThread(HWND hWnd, PBUFFER_OBJ pBuffer)
{

	pBuffer->pNext = NULL;
	::EnterCriticalSection(&g_cs);
	PTHREAD_OBJ pThread = g_pThreadList;
	//试图插入到现存线程
	while (pThread != NULL)
	{
		if (InsertBufferObj(pThread, pBuffer))
			break;
		pThread = pThread->pNext;
	}
	//没有空闲线程，为这个套接字创建新的线程
	if (pThread == NULL)
	{
		pThread = GetThreadObj(hWnd);
		InsertBufferObj(pThread, pBuffer);
		ServerThreadParam *sParam = new ServerThreadParam;
		sParam->hWnd = hWnd;
		sParam->pThread = pThread;
		sParam->test = 1111;
		::CreateThread(NULL, 0, ServerThread, sParam, 0, NULL);
	}
	::LeaveCriticalSection(&g_cs);

	//指示线程重建句柄数组
	::WSASetEvent(pThread->events[0]);

}

//void CServerDlg::RemoveBufferObj(PTHREAD_OBJ pThread, PBUFFER_OBJ pBuffer)
//{
//	::EnterCriticalSection(&pThread->cs);
//	//在缓冲区对象列表中查找指定的缓冲区对象，找到后将之移除
//	PBUFFER_OBJ pTest = pThread->pBufferHead;
//	BOOL bFind = FALSE;
//	if (pTest == pBuffer)
//	{
//		if (pThread->pBufferHead == pThread->pBufferTail)
//			pThread->pBufferTail = pThread->pBufferHead = pTest->pNext;
//		else
//			pThread->pBufferHead = pTest->pNext;
//		bFind = TRUE;
//
//	}
//	else
//	{
//		while (pTest != NULL && pTest->pNext != pBuffer)
//			pTest = pTest->pNext;
//		if (pTest != NULL)
//		{
//			if (pThread->pBufferTail == pBuffer)
//				pThread->pBufferTail = pTest;
//			pTest->pNext = pBuffer->pNext;
//			bFind = TRUE;
//		}
//	}
//	if (bFind) {
//		pThread->nBufferCount--;
//		::CloseHandle(pBuffer->ol.hEvent);
//		::GlobalFree(pBuffer->buff);
//		::GlobalFree(pBuffer);
//		
//		::WSASetEvent(pThread->events[0]);     //指示线程重建句柄数组
//		//::InterlockedDecrement(&g_nCurrentConnections);    //说明一个连接中断????? 在重叠I/O中应该不需要这个吧？？？
//	}
//	::LeaveCriticalSection(&pThread->cs);
//}


//工作线程，负责处理客户的I/O请求
DWORD WINAPI CServerDlg::ServerThread(LPVOID lpVoid)
{
	// Sleep(10000);
	//取得本线程对象的指针和窗口对象
	ServerThreadParam *sParam =(ServerThreadParam*)lpVoid;
	int test = sParam->test;
	HWND hWnd = sParam->hWnd;
	//测试
	::SendMessage(hWnd, WM_MY_THREADNUM, (WPARAM)"创建了线程\n", 0);
	//::SendMessage(hWnd, WM_MY_MESSAGE, (WPARAM)"测试传hWnd和pthread\n", 0);
	PTHREAD_OBJ pThread = sParam->pThread;

	/*HWND hWnd = *((HWND *)lpVoid);
	::SendMessage(hWnd, WM_MY_MESSAGE, (WPARAM)"测试单独传hWnd\n", 0);*/

	while(TRUE)
	{
		//等待网络事件
		int nIndex = ::WSAWaitForMultipleEvents(pThread->nBufferCount + 1, pThread->events, FALSE, WSA_INFINITE, FALSE);
		/*if (nIndex == WSA_WAIT_FAILED) {
			int error = WSAGetLastError();
			::SendMessage(hWnd, WM_MY_MESSAGE, (WPARAM)"WSAWaitForMultipleEvents() failed\n", 0);
			
			break;
		}*/
		nIndex=nIndex-WSA_WAIT_EVENT_0;
		//查看受信的事件对象
		for (int i = nIndex; i < pThread->nBufferCount+1; i++)
		{
			nIndex = ::WSAWaitForMultipleEvents(1, &pThread->events[i], TRUE, 1000, FALSE);
			if (nIndex == WSA_WAIT_TIMEOUT || nIndex==WSA_WAIT_FAILED)
				continue;
			else
			{
				//::WSAResetEvent(pThread->events[i]);
				// 重新建立g_events数组
				if (i == 0)
				{
					RebuildArray(pThread);
					if (pThread->nBufferCount == 0) 
					{
						::SendMessage(hWnd, WM_MY_THREADNUM, (WPARAM)"释放了线程\n", 0);
						FreeThreadObj(hWnd, pThread);
						return 0;
					}
					::WSAResetEvent(pThread->events[0]);
				}
				else {
					// 处理这个I/O
					PBUFFER_OBJ pBuffer = FindBufferObj(pThread,i);
					if (pBuffer != NULL)
					{
						::EnterCriticalSection(&pBuffer->pSocket->s_cs);
						if (!HandleIO(pThread, pBuffer, hWnd))
							RebuildArray(pThread);
						::LeaveCriticalSection(&pBuffer->pSocket->s_cs);
					}
					else
						::SendMessage(hWnd, WM_MY_MESSAGE, (WPARAM)" Unable to find socket object \n ", 0);
						
				}				
			}
		}
	}
	
	return 0;

}

