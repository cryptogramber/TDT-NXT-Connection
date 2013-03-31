
// implementation file
//
// AMBER:
//		-> Fix release() shutdown crash
//		-> Logging
//		-> Remove the current abundance of redundancy in the drive functions
//		-> Clean up wizard implementation (rm Conveyor2Leverv3 syntax) 
//		-> Clean up.. in general. Such shame.
//		-> Add comments that are actually useful.

#include "stdafx.h"
#include "TDTNXTConnectMain.h"
#include "TDTNXTConnectDlg.h"
#include "afxdialogex.h"
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

UINT NXTMotorDriveProc( LPVOID Param );
Comm::NXTComm talkNXT;
TDEVACCXLib::_DTDevAccXPtr talkTDT;
bool experimentGo;
int leftLeverCount;
int rightLeverCount;
CTime theTime;
int experimentType;
bool greenLeverOff;
bool blueLeverOff;
bool pushThosePuffs;
int trialDebounce;
int trialHold;
bool pushAPuff;

// dialog
CBrainConnectDlg::CBrainConnectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CBrainConnectDlg::IDD, pParent)
	, RLCount(_T(""))
	, LLCount(_T(""))
	, startTime(_T(""))
	, currTime(_T(""))
	, NoGreen(FALSE)
	, NoBlue(FALSE)
	, LeverDebounce(0)
	, HoldTime(0)
	, PUFFPUSH(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBrainConnectDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, battLevelNXT, currentBattery);
	DDX_Text(pDX, RL, RLCount);
	DDX_Text(pDX, LL, LLCount);
	DDX_Text(pDX, ES, startTime);
	DDX_Text(pDX, TE, currTime);
	DDX_Control(pDX, ETYPE, SetExperimentType);
	DDX_Check(pDX, GREEN, NoGreen);
	DDX_Check(pDX, BLUE, NoBlue);
	DDX_Text(pDX, DEBOUNCE, LeverDebounce);
	DDV_MinMaxInt(pDX, LeverDebounce, 0, 20000);
	DDX_Text(pDX, HTIME, HoldTime);
	DDV_MinMaxInt(pDX, HoldTime, 0, 20000);
	DDX_Check(pDX, APUSH, PUFFPUSH);
}

BEGIN_MESSAGE_MAP(CBrainConnectDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(STARTE, &CBrainConnectDlg::OnBnClickedStarte)			// start trial
	ON_BN_CLICKED(STOPE, &CBrainConnectDlg::OnBnClickedStope)			// stop trial
	ON_WM_TIMER()														// timer process (background)
	ON_CBN_SELCHANGE(ETYPE, &CBrainConnectDlg::OnCbnSelchangeEtype)		// change in exp type
	ON_BN_CLICKED(SHUTDOWN, &CBrainConnectDlg::OnBnClickedShutdown)		// shutdown program
	ON_BN_CLICKED(GREEN, &CBrainConnectDlg::OnBnClickedGreen)			// disable green lever
	ON_BN_CLICKED(BLUE, &CBrainConnectDlg::OnBnClickedBlue)				// disable blue lever
	ON_EN_CHANGE(DEBOUNCE, &CBrainConnectDlg::OnEnChangeDebounce)		// debounce time
	ON_EN_CHANGE(HTIME, &CBrainConnectDlg::OnEnChangeHtime)				// motor/hold time
	ON_BN_CLICKED(APUSH, &CBrainConnectDlg::OnBnClickedApush)
	ON_BN_CLICKED(MANPUFF, &CBrainConnectDlg::OnBnClickedManpuff)
END_MESSAGE_MAP()

// message handlers
BOOL CBrainConnectDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);												// set big icon
	SetIcon(m_hIcon, FALSE);											// set small icon
	batteryMonitor = 0;													// nxt battery
	currentBattery.Format(_T("%d"), batteryMonitor);					// format for nxt battery
	experimentType = 0;													// L+TDT ; L+Con : L+Con+TDT
	UpdateData(FALSE);
	experimentGo = false;
	refreshTimer = 0;
	SetTimer(0,500,NULL);
	GetDlgItem(STOPE)->EnableWindow(FALSE);								// can't stop before start
	GetDlgItem(MANPUFF)->EnableWindow(FALSE);
	SetExperimentType.SetCurSel(0);										// set L+TDT as default type
	greenLeverOff = false;												// green enabled by default
	blueLeverOff = false;												// blue enabled by default
	pushThosePuffs = false;
	LeverDebounce = 100;												// debounce val in ms
	HoldTime = 800;														// hold time in ms
	return TRUE;  // return TRUE unless the focus is set to a control
}

// note: if you add a minimize button to your dialog, you will need the code below to draw the icon 
void CBrainConnectDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this);								// device context for painting
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);						// draw the icon
	} else {
		CDialogEx::OnPaint();
	}
}

//  system calls this function to obtain the cursor to display while the user drags the minimized window
HCURSOR CBrainConnectDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

// creates the TDT activeX instance, queries the TDT to see if preview/record mode is enabled
// before making write calls to the TDT rex circuit
int createTDTInstance() {
		HRESULT hr;
		hr = CoInitialize(NULL);
		if (FAILED(hr)) {
			TRACE(traceAppMsg, 0, "Failed at ActX HR init.\n");
			return 0;
		} else {
			const char* appId = "{09EFA19D-3AD0-49A8-8232-18D6F7512CE8}"; // TDevAccX.ocx
			hr = talkTDT.CreateInstance(appId);
			if (FAILED(hr)) {
				TRACE(traceAppMsg, 0, "Failed at instance creation.\n");
				return 0;
			} else {
				if ((*talkTDT).ConnectServer("Local")) {
					TRACE(traceAppMsg, 0, "Connected to TDT.\n"); 
					switch ((*talkTDT).GetSysMode()) {
					case 0:	// idle mode
						TRACE(traceAppMsg, 0, "TDT in state 0 (idle mode); halting.\n");
						experimentGo = false;
						break;
					case 1: // standby mode
						TRACE(traceAppMsg, 0, "TDT in state 1 (standby mode); halting.\n");
						experimentGo = false;
						break;
					case 2: // preview mode
						TRACE(traceAppMsg, 0, "TDT in state 2 (preview mode); running!\n");
						(*talkTDT).SetTargetVal("RZ5.StatusC",1);
						experimentGo = true;
						break;
					case 3: // record mode
						TRACE(traceAppMsg, 0, "TDT in state 3 (recording mode); running!.\n");
						(*talkTDT).SetTargetVal("RZ5.StatusC",1);
						experimentGo = true;
						break;
					default: // sad and broken
						TRACE(traceAppMsg, 0, "Something is wrong w/TDT communication.\n");
						experimentGo = false;
						break;
				}
					return 1;
				} else {
					TRACE(traceAppMsg, 0, "Couldn't connect to TDT.\n");

					return 0;
				}
			}
		}
}

void driveMotorNXT() { // drive motor
	int rightLever = 1;
	int leftLever = 1;
	rightLever = NXT::Sensor::GetValue(&talkNXT,IN_1);
	leftLever = NXT::Sensor::GetValue(&talkNXT,IN_2);
	Wait(trialDebounce);
	rightLever = rightLever + NXT::Sensor::GetValue(&talkNXT,IN_1);	// query lever 1
	leftLever = leftLever + NXT::Sensor::GetValue(&talkNXT,IN_2);	// query lever 2		
	if (leftLever == 0 && greenLeverOff == false) {
		//if ((*talkTDT).CheckServerConnection()) TRACE(traceAppMsg, 0, "LeverL: Server Connection Still Established.\n"); else TRACE(traceAppMsg, 0, "LeverL: Server Connection Was Lost.\n");
		if ((*talkTDT).SetTargetVal("RZ5.Lever1",1)) TRACE(traceAppMsg,0,"Lever L successfully set to 1.\n"); else TRACE(traceAppMsg,0,"Couldn't set Lever L to 1.\n");
		(*talkTDT).SetTargetVal("RZ5.Lever1",0);
		//printf("RC (right loop pre): %i\n", NXT::Motor::GetRotationCount(&talkNXT,OUT_A)); 
		leftLeverCount++;
		NXT::Motor::SetForward(&talkNXT,OUT_A,30); // 30% power
		Wait(trialHold); // wait half second
		NXT::Motor::Stop(&talkNXT,OUT_A,false); // stop motor with brake (true) or without (float)
		//printf("RC (right loop post): %i\n", NXT::Motor::GetRotationCount(&talkNXT,OUT_A)); 
		if (pushThosePuffs) {
			NXT::Motor::ResetRotationCount(&talkNXT,OUT_B,false);
			NXT::Motor::GoTo(&talkNXT,OUT_B,25,140,true);
			NXT::Motor::GoTo(&talkNXT,OUT_B,25,0,true);
		}
	} else if (rightLever == 0 && blueLeverOff == false) {
		//if ((*talkTDT).CheckServerConnection()) TRACE(traceAppMsg, 0, "LeverR: Server Connection Still Established.\n"); else TRACE(traceAppMsg, 0, "LeverR: Server Connection Was Lost.\n");
		if ((*talkTDT).SetTargetVal("RZ5.Lever2",3)) TRACE(traceAppMsg,0,"Lever R successfully set to 3.\n"); else TRACE(traceAppMsg,0,"Couldn't set Lever R to 3.\n");
		(*talkTDT).SetTargetVal("RZ5.Lever2",0);
		//printf("RC (left loop pre): %i\n", NXT::Motor::GetRotationCount(&talkNXT,OUT_A)); 
		rightLeverCount++;
		NXT::Motor::SetReverse(&talkNXT,OUT_A,30);
		Wait(trialHold);
		NXT::Motor::Stop(&talkNXT,OUT_A,false);
		//printf("RC (left loop post): %i\n", NXT::Motor::GetRotationCount(&talkNXT,OUT_A)); 
		if (pushThosePuffs) {
			NXT::Motor::ResetRotationCount(&talkNXT,OUT_B,false);
			NXT::Motor::GoTo(&talkNXT,OUT_B,25,140,true);
			NXT::Motor::GoTo(&talkNXT,OUT_B,25,0,true);	
		}
	}
} 

void leversOnlyNXT() { // only levers; no conveyor
	int rightLever = 1;
	int leftLever = 1;
	Wait(trialDebounce);
	rightLever = rightLever + NXT::Sensor::GetValue(&talkNXT,IN_1);	// query lever 1
	leftLever = leftLever + NXT::Sensor::GetValue(&talkNXT,IN_2);	// query lever 2
	if (leftLever == 0 && greenLeverOff == false) {
		//if ((*talkTDT).CheckServerConnection()) TRACE(traceAppMsg, 0, "LeverL: Server Connection Still Established.\n"); else TRACE(traceAppMsg, 0, "LeverL: Server Connection Was Lost.\n");
		if ((*talkTDT).SetTargetVal("RZ5.Lever1",1)) TRACE(traceAppMsg,0,"Lever L successfully set to 1.\n"); else TRACE(traceAppMsg,0,"Couldn't set Lever L to 1.\n");
		(*talkTDT).SetTargetVal("RZ5.Lever1",0);
		leftLeverCount++;
		Wait(trialHold);		// wait 3 seconds (helps reduce epoch stepping on feet)
	} else if (rightLever == 0 && blueLeverOff == false) {
		//if ((*talkTDT).CheckServerConnection()) TRACE(traceAppMsg, 0, "LeverR: Server Connection Still Established.\n"); else TRACE(traceAppMsg, 0, "LeverR: Server Connection Was Lost.\n");
		if ((*talkTDT).SetTargetVal("RZ5.Lever2",3)) TRACE(traceAppMsg,0,"Lever R successfully set to 3.\n"); else TRACE(traceAppMsg,0,"Couldn't set Lever R to 3.\n");
		(*talkTDT).SetTargetVal("RZ5.Lever2",0);
		rightLeverCount++;
		Wait(trialHold);
	}
} 

void leversConveyorNXT() {								// levers + conveyor, no TDT init/involvement
	int rightLever = 1;									// resets rightLever
	int leftLever = 1;									// resets leftLever
	rightLever = NXT::Sensor::GetValue(&talkNXT,IN_1);	// query lever 1
	leftLever = NXT::Sensor::GetValue(&talkNXT,IN_2);	// query lever 2
	Wait(trialDebounce);
	rightLever = rightLever + NXT::Sensor::GetValue(&talkNXT,IN_1);	// query lever 1
	leftLever = leftLever + NXT::Sensor::GetValue(&talkNXT,IN_2);	// query lever 2
	//TRACE(traceAppMsg,0,"rightLever is %d.\n",rightLever);
	//TRACE(traceAppMsg,0,"leftLever is %d.\n",leftLever);
	if (pushAPuff) {
			NXT::Motor::ResetRotationCount(&talkNXT,OUT_B,false);	// reset the rotation count on puff pusher
			NXT::Motor::GoTo(&talkNXT,OUT_B,25,140,true);	// run the puff pusher forward to 140
			NXT::Motor::GoTo(&talkNXT,OUT_B,25,0,true);		// pull the puff pusher back to point 0
			pushAPuff = false;
	}
	if (leftLever == 0 && greenLeverOff == false) {
		leftLeverCount++;							
		NXT::Motor::SetForward(&talkNXT,OUT_A,30);		// run conveyor forward @ 30% power
		Wait(trialHold);										// wait 800ms
		NXT::Motor::Stop(&talkNXT,OUT_A,false);			// stop motor with brake (true) or without (float)
		if (pushThosePuffs) {
			NXT::Motor::ResetRotationCount(&talkNXT,OUT_B,false);	// reset the rotation count on puff pusher
			NXT::Motor::GoTo(&talkNXT,OUT_B,25,140,true);	// run the puff pusher forward to 140
			NXT::Motor::GoTo(&talkNXT,OUT_B,25,0,true);		// pull the puff pusher back to point 0
		}
	} else if (rightLever == 0 && blueLeverOff == false) {
		rightLeverCount++;	
		NXT::Motor::SetReverse(&talkNXT,OUT_A,30);		// run conveyor backward @ 30% power
		Wait(trialHold);										// wait 800ms
		NXT::Motor::Stop(&talkNXT,OUT_A,false);			// stop motor
		if (pushThosePuffs) {
			NXT::Motor::ResetRotationCount(&talkNXT,OUT_B,false);
			NXT::Motor::GoTo(&talkNXT,OUT_B,25,140,true);
			NXT::Motor::GoTo(&talkNXT,OUT_B,25,0,true);	
		}
	}
}

void closeTDTConnection() {
	if ((*talkTDT).CheckServerConnection()) {
		TRACE(traceAppMsg, 0, "Shutting down TDT Connection.\n"); 
		(*talkTDT).SetTargetVal("RZ5.StatusC",0);
		(*talkTDT).CloseConnection();	// CloseConnection is used w/TDevAccX (vs ReleaseServer w/TTankX)
	} else TRACE(traceAppMsg, 0, "Stop: Server Connection Was Lost.\n");					
}

UINT NXTMotorDriveProc( LPVOID Param ) {
	if (experimentType == 0 && createTDTInstance()) {					// levers + TDT
		TRACE(traceAppMsg, 0, "Levers + TDT mode initiated.\n");
		while (experimentGo == true) leversOnlyNXT();
		closeTDTConnection();
	} else if (experimentType == 1) {									// levers + conveyor (no TDT)
		TRACE(traceAppMsg, 0, "Levers + conveyor mode initiated (no TDT).\n");
		while (experimentGo == true) leversConveyorNXT();
	} else if (experimentType == 2 && createTDTInstance()) {			// levers + TDT + conveyor
		TRACE(traceAppMsg, 0, "Levers + TDT + conveyor mode initiated.\n");
		while (experimentGo == true) driveMotorNXT();
		closeTDTConnection();
	} else { 
		experimentGo = false;
		TRACE(traceAppMsg, 0, "Failed in NXTMotorDriveProc.\n");
		if (createTDTInstance() == 0) {
			TRACE(traceAppMsg, 0, "Couldn't init TDT.\n");
		}
	}
	//CoUninitialize();
	NXT::Close(&talkNXT);							// close the NXT
	return 0;
}

void CBrainConnectDlg::OnBnClickedStarte() {
	if(NXT::Open(&talkNXT)) { // initialize the NXT and continue if it succeeds
		TRACE(traceAppMsg, 0, "Successful connection to NXT.\n");
		batteryMonitor = NXT::BatteryLevel(&talkNXT);		// 380 == error
		currentBattery.Format(_T("%d"), batteryMonitor);
		theTime = CTime::GetCurrentTime();
		startTime = theTime.Format("%a, %b %d, %Y, %I:%M %p");
		startTime.Format(_T("%s"), startTime);
		UpdateData(FALSE);
		NXT::Sensor::SetTouch(&talkNXT,IN_1);			// Right Lever
		NXT::Sensor::SetTouch(&talkNXT,IN_2);			// Left Lever
		//TRACE(traceAppMsg, 0, "Lever debounce is currently %i.\n",LeverDebounce);

		trialDebounce = LeverDebounce;
		trialHold = HoldTime;
		rightLeverCount = 0;
		leftLeverCount = 0;
		experimentGo = true;

		AfxBeginThread(NXTMotorDriveProc,NULL,THREAD_PRIORITY_NORMAL,0,0,NULL);
		GetDlgItem(STARTE)->EnableWindow(FALSE);
		GetDlgItem(STOPE)->EnableWindow(TRUE);
		GetDlgItem(SHUTDOWN)->EnableWindow(FALSE);
		GetDlgItem(ETYPE)->EnableWindow(FALSE);
		GetDlgItem(HTIME)->EnableWindow(FALSE);
		GetDlgItem(DEBOUNCE)->EnableWindow(FALSE);
		GetDlgItem(MANPUFF)->EnableWindow(TRUE);
    } else {
		TRACE(traceAppMsg, 0, "Couldn't initialize NXT; try power cycling.\n");
	}
}

void CBrainConnectDlg::OnBnClickedStope() {
	experimentGo = false;							// kills the MotorDriveProc thread
}

void CBrainConnectDlg::OnTimer(UINT_PTR nIDEvent) {
	RLCount.Format(_T("%d"), rightLeverCount);		// increment right lever GUI
	LLCount.Format(_T("%d"), leftLeverCount);		// increment left lever GUI
	if (NoGreen) greenLeverOff = true; else greenLeverOff = false;
	if (NoBlue) blueLeverOff = true; else blueLeverOff = false;
	if (PUFFPUSH) pushThosePuffs = false; else pushThosePuffs = true;
	if (experimentGo == true) {
		CTime elapsedTime = CTime::GetCurrentTime();
		CTimeSpan elapsedTimeCalc = elapsedTime - theTime;
		currTime = elapsedTimeCalc.Format("%H:%M:%S");
		currTime.Format(_T("%s"), currTime);
	} else {
		GetDlgItem(STARTE)->EnableWindow(TRUE);
		GetDlgItem(STOPE)->EnableWindow(FALSE);
		GetDlgItem(ETYPE)->EnableWindow(TRUE);
		GetDlgItem(SHUTDOWN)->EnableWindow(TRUE);
		GetDlgItem(HTIME)->EnableWindow(TRUE);
		GetDlgItem(DEBOUNCE)->EnableWindow(TRUE);
		GetDlgItem(MANPUFF)->EnableWindow(FALSE);
	}
	UpdateData(FALSE);								// won't update GUI elements w/o this
	CDialogEx::OnTimer(nIDEvent);
}

void CBrainConnectDlg::OnCbnSelchangeEtype() {
	experimentType = SetExperimentType.GetCurSel();
	TRACE(traceAppMsg, 0, "New type is: %i\n",experimentType);
}

void CBrainConnectDlg::OnBnClickedShutdown() {
    ASSERT(AfxGetApp()->m_pMainWnd != NULL);
    AfxGetApp()->m_pMainWnd->SendMessage(WM_CLOSE);
}

void CBrainConnectDlg::OnBnClickedGreen() {
	UpdateData(TRUE);
}

void CBrainConnectDlg::OnBnClickedBlue() {
	UpdateData(TRUE);
}

void CBrainConnectDlg::OnEnChangeDebounce() {
	UpdateData(TRUE);
}

void CBrainConnectDlg::OnEnChangeHtime() {
	UpdateData(TRUE);
}

void CBrainConnectDlg::OnBnClickedApush() {
	UpdateData(TRUE);
}

void CBrainConnectDlg::OnBnClickedManpuff() {
	pushAPuff = true;
}
