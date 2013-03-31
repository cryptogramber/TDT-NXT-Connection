
// BrainConnectDlg.h : header file
//

#pragma once

#include "atltime.h"
#include "time.h"
#include "Windows.h"
#include "iostream"
#include "NXT++.h"
#include "afxwin.h"
#import "C:\\TDT\\lib64\\TDevAccX.ocx" 

// CConveyor2Leverv3Dlg dialog
class CBrainConnectDlg : public CDialogEx
{
// Construction
public:
	CBrainConnectDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_BRAINCONNECT_DIALOG };

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
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedStarte();
	afx_msg void OnBnClickedStope();
	afx_msg void OnCbnSelchangeEtype();
	afx_msg void OnBnClickedShutdown();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CComboBox SetExperimentType;
	afx_msg void OnBnClickedGreen();
	afx_msg void OnBnClickedBlue();
	afx_msg void OnEnChangeDebounce();
private:
	CString currentBattery;
	CString RLCount;
	CString LLCount;
	CString startTime;
	int batteryMonitor;
	int refreshTimer;	
	CString currTime;
	BOOL NoGreen;
	BOOL NoBlue;
	int HoldTime;
	int LeverDebounce;
public:
	afx_msg void OnEnChangeHtime();
private:
	BOOL PUFFPUSH;
public:
	afx_msg void OnBnClickedApush();
	afx_msg void OnBnClickedManpuff();
};