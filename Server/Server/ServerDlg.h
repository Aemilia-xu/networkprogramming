
// ServerDlg.h: 头文件
//

#include <winsock2.h>
#include <Mswsock.h>
#include <stdio.h>

#pragma once

typedef struct _SOCKET_OBJ SOCKET_OBJ, * PSOCKET_OBJ;
// 缓冲区对象
typedef struct _BUFFER_OBJ
{
	OVERLAPPED ol;			// 重叠结构
	char* buff;				// send/recv/AcceptEx所使用的缓冲区
	int nLen;				// buff的长度
	PSOCKET_OBJ pSocket;	// 此I/O所属的套节字对象
	ULONG nSequenceNumber; // 此 I/O 的序列号

	int nOperation;			// 提交的操作类型
#define OP_ACCEPT	1
#define OP_READ		2
#define OP_WRITE	3

	SOCKET sAccept;			// 用来保存AcceptEx接受的客户套节字（仅对监听套节字而言）
	_BUFFER_OBJ* pNext;
	CRITICAL_SECTION s_cs;
} BUFFER_OBJ, * PBUFFER_OBJ;

// 套接字对象
struct _SOCKET_OBJ
{
	SOCKET s;						// 套节字句柄
	int nOutstandingOps;			// 记录此套节字上的重叠I/O数量
	SOCKADDR_IN addrLocal;          //本地地址
	SOCKADDR_IN addrRemote;         //客户地址
	LPFN_ACCEPTEX lpfnAcceptEx;		// 扩展函数AcceptEx的指针（仅对监听套节字而言）
	CRITICAL_SECTION s_cs;
	LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs;
	_SOCKET_OBJ* pNext;

	ULONG nReadSequence; // 安排给接收的下一个序列号
	ULONG nCurrentReadSequence; // 当前要读的序列号
	_BUFFER_OBJ* pOutOfOrderReads; // 记录没有按顺序完成的读 I/O
};


//线程对象
typedef struct _THREAD_OBJ
{
	HANDLE events[WSA_MAXIMUM_WAIT_EVENTS]; //记录当前线程要等待的I/O事件对象句柄
	int nBufferCount;                       //I/O事件句柄数组中有效句柄数量
	PBUFFER_OBJ pBufferHead;                 //当前线程处理的套接字对象列表，pBufferHeader指向表头
	PBUFFER_OBJ pBufferTail;                 //pBufferTail指向表尾
	CRITICAL_SECTION cs;                    //关键代码段变量，为的是同步对本结构的访问
	_THREAD_OBJ *pNext;                     //指向下一个THREAD_OBJ对象，为的是连成一个表
}THREAD_OBJ, *PTHREAD_OBJ;


// CServerDlg 对话框
class CServerDlg : public CDialogEx
{
// 构造
public:
	CServerDlg(CWnd* pParent = nullptr);// 标准构造函数


// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	static PSOCKET_OBJ GetSocketObj(SOCKET s);
	static void AddSocketOBJ(HWND hWnd, PSOCKET_OBJ pSocket);
	static void FreeSocketObj(HWND hWnd, PSOCKET_OBJ pSocket); //已修改
	static PBUFFER_OBJ GetBufferObj(HWND hWnd,PSOCKET_OBJ pSocket, ULONG nLen);  //已修改
	static void FreeBufferObj(PTHREAD_OBJ pThread, PBUFFER_OBJ pBuffer);  //已修改
	static PBUFFER_OBJ FindBufferObj(PTHREAD_OBJ pThread, int nIndex); //已修改
	//static void RebuildArray();
	static BOOL PostAccept(PBUFFER_OBJ pBuffer); //已修改
	static BOOL PostRecv(PSOCKET_OBJ pSocket, PBUFFER_OBJ pBuffer);//已修改
	static BOOL PostSend(PBUFFER_OBJ pBuffer);//已修改
	static BOOL HandleIO(PTHREAD_OBJ pThread, PBUFFER_OBJ pBuffer, HWND hWnd); //已修改
	static UINT RunServer(LPVOID lpVoid);    //修改
	static BOOL SendFile(PBUFFER_OBJ pBuffer, HWND hWnd);//发送文件
	//申请和释放线程对象的函数
	static PTHREAD_OBJ GetThreadObj(HWND hWnd);
	static void FreeThreadObj(HWND hWnd,PTHREAD_OBJ pThread);
	static void RebuildArray(PTHREAD_OBJ pThread);
	static BOOL InsertBufferObj(PTHREAD_OBJ pThread, PBUFFER_OBJ pBuffer);
	static void AssignToFreeThread(HWND hWnd, PBUFFER_OBJ pBuffer);
	//static void RemoveBufferObj(PTHREAD_OBJ pThread, PBUFFER_OBJ pBuffer);
	static DWORD WINAPI ServerThread(LPVOID lpVoid);
	// 消息处理函数
	afx_msg LRESULT OnMyMessage(WPARAM wParam, LPARAM lParam);
	//更新线程数量，打印线程状态
	afx_msg LRESULT PrintThread(WPARAM wParam, LPARAM lParam);
	//更新socket连接信息
	afx_msg LRESULT PrintSocket(WPARAM wParam, LPARAM lParam);
	//更新文件传输状态
	afx_msg LRESULT OnMyFile(WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()
private:
	CListBox threadNum;
	CListBox connectionList;
	CListBox clientList;
	CListBox threadInfo;
	CListBox dialog;
	CListBox fileStatus;

};
