#include "MicrowaveStateMachine.h"
// MicrowaveDlg.h : header file
//

#pragma once

// CMicrowaveDlg dialog
class CMicrowaveDlg : public CDialogEx
{
	int maxTimerValue;
	int minTimerValue;

	CRichEditCtrl* log;
	CSpinButtonCtrl* spin;
	std::wstring endline;
	MicrowaveStateMachine* microwave;

	std::mutex m;

	class CallbackTimer: public CallbackFunc
	{
		CRichEditCtrl* log;
		MicrowaveStateMachine* sm;
		CSpinButtonCtrl* spin;
		std::wstring endline;
		std::mutex& m;

		public:
			CallbackTimer(CRichEditCtrl* _log, MicrowaveStateMachine* _sm, CSpinButtonCtrl* _spin, std::mutex& _m, std::wstring _endline): 
				log(_log), sm(_sm), spin(_spin), m(_m), endline(_endline) {}
			void operator()();
			~CallbackTimer(){}
	};

	CallbackTimer* callbackTimer;
// Construction
public:
	CMicrowaveDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MICROWAVE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonOpenDoor();
	afx_msg void OnBnClickedButtonCloseDoor();
	afx_msg void OnBnClickedButtonStart();
	std::wstring getMsg(std::wstring eventName, MicrowaveStateMachine* ms, bool isPossible);
	~CMicrowaveDlg() {delete microwave; delete callbackTimer;}
	afx_msg void OnDeltaposSpinTimer(NMHDR *pNMHDR, LRESULT *pResult);
};
