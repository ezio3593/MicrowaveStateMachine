
// MicrowaveDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Microwave.h"
#include "MicrowaveDlg.h"
#include "afxdialogex.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMicrowaveDlg dialog

CMicrowaveDlg::CMicrowaveDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMicrowaveDlg::IDD, pParent)
{
	microwave = NULL;
	callbackTimer = NULL;
	log = NULL;
	spin = NULL;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	endline = L"\r\n";
	maxTimerValue = 60;
	minTimerValue = 0;
}

void CMicrowaveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMicrowaveDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OPEN_DOOR, &CMicrowaveDlg::OnBnClickedButtonOpenDoor)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE_DOOR, &CMicrowaveDlg::OnBnClickedButtonCloseDoor)
	ON_BN_CLICKED(IDC_BUTTON_START, &CMicrowaveDlg::OnBnClickedButtonStart)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TIMER, &CMicrowaveDlg::OnDeltaposSpinTimer)
END_MESSAGE_MAP()

// CMicrowaveDlg message handlers

BOOL CMicrowaveDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	spin = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_TIMER);
	spin->SetRange(minTimerValue, maxTimerValue);
	log = (CRichEditCtrl*)GetDlgItem(IDC_EDIT_LOG);
	microwave = new MicrowaveStateMachine();
	callbackTimer = new CallbackTimer(log, microwave, spin, m, endline);
	microwave->setCallbackFunc(callbackTimer);

	std::wstring msg = L"Initial state ";
	msg += microwave->getCurrentStateName();
	msg += endline;

	int end = log->GetWindowTextLength(); 
	log->SetSel(end, end);
	log->ReplaceSel(msg.c_str());

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMicrowaveDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMicrowaveDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMicrowaveDlg::OnBnClickedButtonOpenDoor()
{
	std::lock_guard<std::mutex> lk(m);

	int end = log->GetWindowTextLength(); 
	log->SetSel(end, end);

	try 
	{
		microwave->openDoor();
		std::wstring msg = getMsg(L"Event OPEN_DOOR ", microwave, true);
		if (microwave->getTimerValue() != spin->GetPos()) 
		{
			spin->SetPos(microwave->getTimerValue());
			msg += L"Reset timer";
			msg += endline;
		}
		log->ReplaceSel(msg.c_str());
	} catch(const ImpossibleEventException&) 
	{
		log->ReplaceSel(getMsg(L"Event OPEN_DOOR ", microwave, false).c_str());
	};
}


void CMicrowaveDlg::OnBnClickedButtonCloseDoor()
{
	std::lock_guard<std::mutex> lk(m);

	int end = log->GetWindowTextLength();
	log->SetSel(end, end);

	try 
	{
		microwave->closeDoor();
		log->ReplaceSel(getMsg(L"Event CLOSE_DOOR ", microwave, true).c_str());
	} catch(const ImpossibleEventException&)
	{
		log->ReplaceSel(getMsg(L"Event CLOSE_DOOR ", microwave, false).c_str());
	};
}


void CMicrowaveDlg::OnBnClickedButtonStart()
{
	std::lock_guard<std::mutex> lk(m);

	int end = log->GetWindowTextLength();
	log->SetSel(end, end);
	
	try
	{
		microwave->start();

		std::wstring msg = getMsg(L"Event START ", microwave, true);

		msg += L"Wait ";
		msg += std::to_wstring(microwave->getTimerValue());
		msg += L" sec";
		msg += endline;

		log->ReplaceSel(msg.c_str());
	} catch (const ImpossibleEventException&)
	{
		log->ReplaceSel(getMsg(L"Event START ", microwave, false).c_str());
	}
}

void CMicrowaveDlg::OnDeltaposSpinTimer(NMHDR *pNMHDR, LRESULT *pResult)
{
	std::lock_guard<std::mutex> lk(m);

	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	int pos = pNMUpDown->iPos;
	int delta = pNMUpDown->iDelta;

	pos = (pos == minTimerValue && delta < 0) || (pos == maxTimerValue && delta > 0) ? pos : pos + delta;

	try
	{
		microwave->setTimer(pos);

		std::wstring msg = getMsg(L"Event SET_TIMER ", microwave, true);
		
		if (microwave->getTimerValue() == pos)
		{
			msg += L"Timer set on ";
			msg += std::to_wstring(pos);
			msg += L" sec";
			msg += endline;
		} else 
		{
			msg += L"Reset timer";
			msg += endline;
			spin->SetPos(microwave->getTimerValue());
		}
		log->ReplaceSel(msg.c_str());
	} catch (const ImpossibleEventException&) 
	{
		log->ReplaceSel(getMsg(L"Event SET_TIMER ", microwave, false).c_str());
	}
} 

void CMicrowaveDlg::CallbackTimer::operator()()
{
	std::lock_guard<std::mutex> lk(m);

	int end = log->GetWindowTextLength();
	log->SetSel(end, end);

	std::wstring msg = L"Event TIME_OUT ";
	msg += L"go to the state ";
	msg += sm->getCurrentStateName();
	msg += endline;
	msg += L"Reset timer";
	msg += endline;

	spin->SetPos(sm->getTimerValue());
	log->ReplaceSel(msg.c_str());
}

std::wstring CMicrowaveDlg::getMsg(std::wstring eventName, MicrowaveStateMachine* ms, bool isPossible)
{
	std::wstring msg = eventName;
	
	if (ms)
	{
		if (isPossible)
		{
			msg += L"go to the state ";
			msg += ms->getCurrentStateName();
			msg += endline;
		}
		else
		{
			msg += L"impossible in state ";
			msg += ms->getCurrentStateName();
			msg += endline;
		}

		return msg;
	} else return L"";
}
