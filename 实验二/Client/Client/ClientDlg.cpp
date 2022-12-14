
// ClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "Client.h"
#include "ClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define SERVER_ADDRESS "192.168.1.101"
#define PORT           4567
#define MSGSIZE        1024

#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")
// 自定义消息id 
#define WM_MY_MESSAGE (WM_USER+100)
#define WM_MY_FILE (WM_USER+103)
struct threadParam
{
	HWND hWnd;
	SOCKET m_socket;
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


// CClientDlg 对话框
CClientDlg::CClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT1, editSendContent);
	DDX_Control(pDX, IDC_LIST1, lbChatContent);
	DDX_Control(pDX, IDC_LIST2, FileBox);
}

BEGIN_MESSAGE_MAP(CClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	ON_BN_CLICKED(IDOK, &CClientDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CClientDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON1, &CClientDlg::OnBnClickedButton1)

	// 映射消息处理函数 
	ON_MESSAGE(WM_MY_MESSAGE, &CClientDlg::OnMyMessage)
	ON_MESSAGE(WM_MY_FILE, &CClientDlg::OnMyFile)
END_MESSAGE_MAP()


// CClientDlg 消息处理程序

BOOL CClientDlg::OnInitDialog()
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

	// TODO: 在此添加额外的初始化代码
	// WSAStartup
	BYTE minorVer = 2;
	BYTE majorVer = 2;
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(minorVer, majorVer);
	if (::WSAStartup(sockVersion, &wsaData) != 0)
	{
		exit(0);
	}

	// 创建client socket
	sClient = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == INVALID_SOCKET) {
		MessageBox(_T("Failed socket()\n"));
		return false;
	}

	//填充sockaddr_in结构
	SOCKADDR_IN server;
	memset(&server, 0, sizeof(SOCKADDR_IN));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.S_un.S_addr = inet_addr(SERVER_ADDRESS);

	//将套接字与服务器连接
	if (::connect(sClient, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
		MessageBox(_T("Failed connect()\n"));
		int error = WSAGetLastError();
		return FALSE;
	}

	struct threadParam tParam;
	tParam.hWnd = AfxGetMainWnd()->m_hWnd;
	tParam.m_socket = sClient;
	//将RecvProc作为子线程运行，进行监听
	RecvThread = AfxBeginThread(RecvProc, (LPVOID)&tParam);
	Sleep(1000);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CClientDlg::OnPaint()
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

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// listbox消息处理函数
LRESULT CClientDlg::OnMyMessage(WPARAM wParam, LPARAM lParam) {
	// 类型转换
	CString str;
	str = (char*)wParam;
	// 显示
	int count = lbChatContent.GetCount();//获得行数，在之后加上发送的内容
	lbChatContent.InsertString(count, _T("127.0.0.1:\n"));
	lbChatContent.InsertString(count + 1, str);
	return 0;
}

// file listbox消息处理函数
LRESULT CClientDlg::OnMyFile(WPARAM wParam, LPARAM lParam) {
	// 类型转换
	CString str;
	str = (char*)wParam;
	// 显示
	int count = FileBox.GetCount();//获得行数，在之后加上发送的内容
	FileBox.InsertString(count, str);
	return 0;
}


// Send message按钮
void CClientDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CString buffer;
	char* send_buf = NULL, * send_ip = NULL;
	int send_len = 0;
	//从edit control中读入要发送的文字信息
	editSendContent.GetWindowTextW(buffer);
	send_len = buffer.GetLength();

	USES_CONVERSION;
	send_buf = T2A(buffer);

	//将发送的内容更新在list box中
	if (send_len > 0) {
		int count = lbChatContent.GetCount();//获得行数，在之后加上发送的内容

		lbChatContent.InsertString(count, _T("本机:"));
		lbChatContent.InsertString(count + 1, buffer);

		if (send(sClient, send_buf, strlen(send_buf), 0) == SOCKET_ERROR) {
			MessageBox(_T("Send message failed.\n"));
		}
	}

	//CDialogEx::OnOK();
}

UINT CClientDlg::RecvProc(LPVOID lpVoid) {
	// 获取参数
	HWND hWnd = ((struct threadParam*)lpVoid)->hWnd;
	SOCKET m_socket = ((struct threadParam*)lpVoid)->m_socket;

	if (m_socket == INVALID_SOCKET)
	{
		AfxMessageBox(_T("RecvProc Failed socket()\n"));
		return 0;
	}

	// 接收数据
	char msg[MSGSIZE];
	int res;
	char FileLen[MSGSIZE+1]; //文件的大小
	const char* filename = "./res/receive.txt";
	char buffer[MSGSIZE];
	while (TRUE)
	{
		int nRecv = ::recv(m_socket, msg, 1024, 0);
		if (nRecv > 0)
		{
			msg[nRecv] = '\0';
			if (strcmp(msg, "传输开始") == 0) {//开始接收文件
				::PostMessage(hWnd, WM_MY_FILE, (WPARAM)msg, 0);
				// 先接收文件大小
				if ((res = ::recv(m_socket, FileLen, 1024, NULL)) > 0)
				{
					FileLen[res] = '\0';
					unsigned int file_size = strtol(FileLen, NULL, 10);	// 文件大小从char*类型转换

					// 创建文件句柄
					HANDLE hFile;
					hFile = CreateFile(CString(filename), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					DWORD dwNumberOfBytesRecv = 0;
					DWORD dwCountOfBytesRecv = 0;
					//接收文件
					do
					{
						memset(buffer, 0, MSGSIZE);
						dwNumberOfBytesRecv = ::recv(m_socket, buffer, sizeof(buffer), 0);
						char bcopy[MSGSIZE*2];
						strcpy(bcopy, buffer);
						bcopy[dwNumberOfBytesRecv]= '\0';
						if (strcmp(bcopy, "传输结束") == 0) {//收到传输结束
							CloseHandle(hFile);
							// AfxMessageBox(_T("接收结束"));
							Sleep(500);
							::PostMessage(hWnd, WM_MY_FILE, (WPARAM)bcopy, 0);
							break;
						}
						else {//还在传文件
							::WriteFile(hFile, buffer, dwNumberOfBytesRecv, &dwNumberOfBytesRecv, NULL);
							dwCountOfBytesRecv += dwNumberOfBytesRecv;
						}
					} while (file_size - dwCountOfBytesRecv);
				}

			}
			else {//传输文字
				::PostMessage(hWnd, WM_MY_MESSAGE, (WPARAM)msg, 0);
			}

		}
	}
}

// send file按钮
void CClientDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码

	// 首先向客户端请求传输文件
	char buffer[MSGSIZE];
	const char* str = "请求发送文件";
	//发送准备发送命令
	memset(buffer, 0, MSGSIZE);
	strncpy(buffer, str, strlen(str));
	if (send(sClient, buffer, strlen(str), 0) == SOCKET_ERROR)
	{
		MessageBox(_T("Request failed.\n"));
		return;
	}
}

// cancel按钮
void CClientDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	//关闭套接字
	::closesocket(sClient);
	//释放Winsock库
	WSACleanup();
	//关闭程序
	CDialogEx::OnCancel();

}

