#include <afxwin.h>
#include <Winuser.h>

class MainWindow :public CFrameWnd
{
public:
	MainWindow()
	{
		Create(NULL, _T("GPGPU test"));
	}
	BOOL PreCreateWindow(CREATESTRUCT& cs)
	{
		int screenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYFULLSCREEN);
		cs.cy = 240; // width
		cs.cx = 320; // height
		cs.y = (screenHeight - cs.cy)/2; // top position
		cs.x = (screenWidth - cs.cx)/2; // left position

		if (!CFrameWnd::PreCreateWindow(cs))
			return FALSE;
		// TODO: Modify the Window class or styles here by modifying
		//  the CREATESTRUCT cs

		//my edits here
		cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

		cs.style &= (0xFFFFFFFF ^ WS_SIZEBOX);
		cs.style |= WS_BORDER;
		cs.style &= (0xFFFFFFFF ^ WS_MAXIMIZEBOX);
		//end my edits

		cs.lpszClass = AfxRegisterWndClass(0);

		// don't forget to call base class version, suppose you derived you window from CWnd
		return CFrameWnd::PreCreateWindow(cs);
	}

};

class MainDialog : public CDialog {
	CButton *openButton;
	BOOL OnInitDialog()
	{
		CDialog::OnInitDialog();

		openButton = new CButton();
		openButton->Create(_T("Open Image"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(40, 40, 260, 160), this, 1);

		return TRUE;
	}
};

class Main :public CWinApp
{
	MainWindow *wnd;
	MainDialog *dialog;
public:
	BOOL InitInstance()
	{
		wnd = new MainWindow();
		dialog = new MainDialog();
		dialog->Create(103, wnd);

		m_pMainWnd = wnd;
		m_pMainWnd->ShowWindow(SW_SHOW);
		return TRUE;
	}
};

Main main;