
// ClientDlg.h: 头文件
//

#pragma once


// CClientDlg 对话框
class CClientDlg : public CDialogEx
{
// 构造
public:
	CClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CLIENT_DIALOG };
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
	DECLARE_MESSAGE_MAP()

public:
	// 编辑框
	CEdit editSendContent;
	// listbox对应的变量，输出接收和发送的信息
	CListBox lbChatContent;
	// send message按钮
	afx_msg void OnBnClickedOk();
	// send file按钮
	afx_msg void OnBnClickedButton1();
	// cancel按钮
	afx_msg void OnBnClickedCancel();

	// 消息处理函数
	afx_msg LRESULT OnMyMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnMyFile(WPARAM wParam, LPARAM lParam);
	//接收函数，实现监听、接收消息的功能
	static UINT RecvProc(LPVOID lpVoid);

	SOCKET sClient;
	CWinThread* RecvThread;
	// 文件传输状态显示框
	CListBox FileBox;
};
