// CONEMU_Win.cpp : �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"
#include <CommCtrl.h>
#include "ftd2xx.h"
#include "CONEMU_Win.h"

#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100

// USB �̍ő�o�b�t�@�T�C�Y
#define MAX_BUFSIZE 64000

// ��M�p�����O�o�b�t�@�̃T�C�Y
#define MAX_RINGBUF 64000

// �E�B���h�E�̃^�C�g���̍���
#define TITLE_HEIGHT (GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU))

#define FPS60 0
#define FPS30 1
#define FPS15 3
#define FPS07 7
#define RowBytes 651							// ROW �o�C�g
#define Lines 400								// ���C����
#define ID_STATUS (100)							// �X�e�[�^�X�o�[�̂h�c

// �O���[�o���ϐ�:
HINSTANCE hInst;								// ���݂̃C���^�[�t�F�C�X
HWND hWnd;										// �E�B���h�E
HWND hStatusBar;								// �X�e�[�^�X�o�[
HANDLE hEvent;									// USB��M�C�x���g�̃n���h��
HANDLE hRxThread;								// ��M�X���b�h�̃n���h��
DWORD ThRxID;									// ��M�X���b�hID
TCHAR szTitle[MAX_LOADSTRING];					// �^�C�g�� �o�[�̃e�L�X�g
TCHAR szWindowClass[MAX_LOADSTRING];			// ���C�� �E�B���h�E �N���X��
HMENU hMenu;									// ���j���[�̃n���h��
MENUITEMINFO menuInfo;							// MENUITEMINFO�\����
int Offset_Y2 = 34;								// Y���I�t�Z�b�g(200���C���p)
int Offset_Y4 = 34;								// Y���I�t�Z�b�g(400���C���p)
int Offset_X2 = 20;								// X���I�t�Z�b�g(200���C���p)
int Offset_X4 = 20;								// X���I�t�Z�b�g(400���C���p)
FT_HANDLE ftHandle;								// USB �n���h��
BITMAPINFO g_biDIB;								// DIB�p�ϐ�
LPDWORD g_lppxDIB = NULL;
BYTE lbuf[650];									// USB�����荞��1���C�����̃f�[�^
BYTE RxBuff[MAX_RINGBUF];						// ��M�p�����O�o�b�t�@
DWORD rptr = 0;									// �����O�o�b�t�@�p�ǂݏo���|�C���^
DWORD wptr = 0;									// �����O�o�b�t�@�p�������݃|�C���^
DWORD bptr = 0;									// ���C���o�b�t�@�p�������݃|�C���^
BYTE BOXmode = 0x80;							// �L���v�`���{�b�N�X�̓����� 0:��~ 0x80:����
BYTE FPSmode = 7;								// �t���[�����[�g 7:7.5fps 3:15fps 1:30fps 0:60fps
BOOL L200;										// ��ʃ��[�h TRUE:200���C�� FALSE:400���C��
DWORD pval[8] = { 0x000000, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF };
LPDWORD ppos[400];
BOOL Pushed = FALSE;
BOOL Released = FALSE;
BOOL KNLocked = FALSE;
BOOL MouseOn = FALSE;
BYTE LBTN = 0;
BYTE RBTN = 0;
POINT mp, cp;
BYTE GRtable[128] = {
	// 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //0x00-0F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //0x10-1F
	0x20, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFD, 0x89, 0x87, 0x8C, 0x88, 0xFE, //0x20-2F
	0xFA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFD, 0x89, 0x87, 0x8C, 0x88, 0xFE, //0x30-3F
	0x8A, 0x7F, 0x84, 0x82, 0xEA, 0xE2, 0xEB, 0xEC, 0xED, 0xE7, 0xEE, 0xEF, 0x8E, 0x86, 0x85, 0xF0, //0x40-4F
	0x8D, 0xE0, 0xE3, 0xE9, 0xE4, 0xE6, 0x83, 0xE1, 0x81, 0xE5, 0x80, 0xFC, 0xFB, 0xE8, 0x8B, 0xFF, //0x50-5F
	0x8A, 0x7F, 0x84, 0x82, 0xEA, 0xE2, 0xEB, 0xEC, 0xED, 0xE7, 0xEE, 0xEF, 0x8E, 0x86, 0x85, 0xF0, //0x60-6F
	0x8D, 0xE0, 0xE3, 0xE9, 0xE4, 0xE6, 0x83, 0xE1, 0x81, 0xE5, 0x80, 0xFC, 0xFB, 0xE8, 0x00, 0x00 }; //0x70-7F
BYTE KNtable[128] = {
	// 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //0x00-0F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //0x10-1F
	0x20, 0xC7, 0xCC, 0xA7, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xB9, 0xDA, 0xC8, 0xCE, 0xD9, 0xD2, //0x20-2F
	0xDC, 0xC7, 0xCC, 0xB1, 0xB3, 0xB4, 0xB5, 0xD4, 0xD5, 0xD6, 0xB9, 0xDA, 0x87, 0x8C, 0x88, 0xA5, //0x30-3F
	0xC1, 0x7F, 0xBA, 0x82, 0xBC, 0xA8, 0xBF, 0xB7, 0xB8, 0xC6, 0xCF, 0xC9, 0xD8, 0xD3, 0xD0, 0xD7, //0x40-4F
	0xBE, 0xC0, 0xBD, 0xC4, 0xB6, 0xC5, 0xCB, 0xC3, 0xBB, 0xDD, 0xAF, 0xDF, 0xB0, 0xD1, 0xCD, 0xDB, //0x50-5F
	0xC1, 0x7F, 0xBA, 0x82, 0xBC, 0xB2, 0xBF, 0xB7, 0xB8, 0xC6, 0xCF, 0xC9, 0xD8, 0xD3, 0xD0, 0xD7, //0x60-6F
	0xBE, 0xC0, 0xBD, 0xC4, 0xB6, 0xC5, 0xCB, 0xC3, 0xBB, 0xDD, 0xC2, 0xA2, 0xFB, 0xA3, 0x00, 0x00 }; //0x70-7F
BYTE KCtable[128] = {
	// 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //0x00-0F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //0x10-1F
	0x20, 0xC7, 0xCC, 0xA7, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xB9, 0xDA, 0xC8, 0xCE, 0xD9, 0xD2, //0x20-2F
	0xDC, 0xC7, 0xCC, 0xB1, 0xB3, 0xB4, 0xB5, 0xD4, 0xD5, 0xD6, 0xB9, 0xDA, 0x87, 0x8C, 0x88, 0xA5, //0x30-3F
	0xC1, 0x7F, 0xBA, 0x82, 0xBC, 0xB2, 0xBF, 0xB7, 0xB8, 0xC6, 0xCF, 0xC9, 0xD8, 0xD3, 0xD0, 0xD7, //0x40-4F
	0xBE, 0xC0, 0xBD, 0xC4, 0xB6, 0xC5, 0xCB, 0xC3, 0xBB, 0xDD, 0xC2, 0xDF, 0xB0, 0xD1, 0xCD, 0xDB, //0x50-5F
	0xC1, 0x7F, 0xBA, 0x82, 0xBC, 0xA8, 0xBF, 0xB7, 0xB8, 0xC6, 0xCF, 0xC9, 0xD8, 0xD3, 0xD0, 0xD7, //0x60-6F
	0xBE, 0xC0, 0xBD, 0xC4, 0xB6, 0xC5, 0xCB, 0xC3, 0xBB, 0xDD, 0xAF, 0xA2, 0xFB, 0xA3, 0x00, 0x00 }; //0x70-7F
//int log[450];

// ���̃R�[�h ���W���[���Ɋ܂܂��֐��̐錾��]�����܂�:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
int					InitUSB();
void				TrashUSB();
DWORD CALLBACK		GetUSB(void *);
void				reallocDIB();
DWORD				ConvPixel();
void				MenuUnchecked(BYTE);
bool				MakeIniFileName(TCHAR *ini_fname, TCHAR *ini_fullpath);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	//HACCEL hAccelTable;
	int bnum;
	RECT screen;
	TCHAR ini_fname[256];

	// �O���[�o������������������Ă��܂��B
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CONEMU_WIN, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// �\���̈�̒�`
	// �K�v�ȊO�̍ď���������h�~����
	screen.left = 0;
	screen.right = 650;
	screen.top = 0;
	screen.bottom = 399;

	// ini �t�@�C������ݒ��ǂ�
	if (MakeIniFileName(_T("CONEMU.ini"), ini_fname)) {
		int value;
		value = GetPrivateProfileInt(_T("FRAMERATE"), _T("FPS"), 7, ini_fname);
		switch (value)
		{
		case 60:
			FPSmode = FPS60;
			break;
		case 30:
			FPSmode = FPS30;
			break;
		case 15:
			FPSmode = FPS15;
			break;
		default:
			FPSmode = FPS07;
			break;
		}
		Offset_X2 = GetPrivateProfileInt(_T("200LINES"), _T("OFFSET_X"), 20, ini_fname);
		Offset_Y2 = GetPrivateProfileInt(_T("200LINES"), _T("OFFSET_Y"), 34, ini_fname);
		Offset_X4 = GetPrivateProfileInt(_T("400LINES"), _T("OFFSET_X"), 20, ini_fname);
		Offset_Y4 = GetPrivateProfileInt(_T("400LINES"), _T("OFFSET_Y"), 34, ini_fname);
	}

	// USB ������
	if (InitUSB()) {
		MessageBox(NULL, _T("�������Ɏ��s���܂����B\r\n�P�[�u���̐ڑ����m�F���Ă��������B"), _T("�G���["), MB_OK);
		return 1;
	}

	reallocDIB();
	ImmDisableIME(-1);

	// �A�v���P�[�V�����̏����������s���܂�:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	// ��ʕ`��|�W�V�����e�[�u��
	for (size_t i = 0; i < 400; i++)
	{
		ppos[i] = g_lppxDIB + RowBytes * ((Lines - 1) - i);
	}

	// ���C�� ���b�Z�[�W ���[�v:
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!(GetMessage(&msg, NULL, 0, 0)))
			{
				// USB ���
				TrashUSB();

				// ini �t�@�C���ɐݒ����������
				if (MakeIniFileName(_T("CONEMU.ini"), ini_fname)) {
					TCHAR cstr[10];
					switch (FPSmode)
					{
					case FPS60:
						wsprintf(cstr, _T("60"));
						break;
					case FPS30:
						wsprintf(cstr, _T("30"));
						break;
					case FPS15:
						wsprintf(cstr, _T("15"));
						break;
					case FPS07:
						wsprintf(cstr, _T("07"));
						break;
					default:
						break;
					}
					WritePrivateProfileString(_T("FRAMERATE"), _T("FPS"), cstr, ini_fname);
					wsprintf(cstr, _T("%d"), Offset_X2);
					WritePrivateProfileString(_T("200LINES"), _T("OFFSET_X"), cstr, ini_fname);
					wsprintf(cstr, _T("%d"), Offset_Y2);
					WritePrivateProfileString(_T("200LINES"), _T("OFFSET_Y"), cstr, ini_fname);
					wsprintf(cstr, _T("%d"), Offset_X4);
					WritePrivateProfileString(_T("400LINES"), _T("OFFSET_X"), cstr, ini_fname);
					wsprintf(cstr, _T("%d"), Offset_Y4);
					WritePrivateProfileString(_T("400LINES"), _T("OFFSET_Y"), cstr, ini_fname);
				}

				return (int)msg.wParam;
			}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
		}
		else
		{
			bnum = ConvPixel();
			switch (bnum)
			{
			case 0:		// �܂�1���C�����̃f�[�^�������ĂȂ�
				break;
			case 399:	// �ŉ��s�̃f�[�^����M
				InvalidateRect(hWnd, &screen, FALSE);
				break;
			default:
				break;
			}
		}
	}

}



//
//  �֐�: MyRegisterClass()
//
//  �ړI: �E�B���h�E �N���X��o�^���܂��B
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;	// �X�^�C��
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CONEMU_WIN));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH); // �E�B���h�E�w�i
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_CONEMU_WIN);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   �֐�: InitInstance(HINSTANCE, int)
//
//   �ړI: �C���X�^���X �n���h����ۑ����āA���C�� �E�B���h�E���쐬���܂��B
//
//   �R�����g:
//
//        ���̊֐��ŁA�O���[�o���ϐ��ŃC���X�^���X �n���h����ۑ����A
//        ���C�� �v���O���� �E�B���h�E���쐬����ѕ\�����܂��B
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // �O���[�o���ϐ��ɃC���X�^���X�������i�[���܂��B

   hWnd = CreateWindow(
	   szWindowClass,
	   szTitle,
	   WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,   // �E�B���h�E�̎��
	   CW_USEDEFAULT,
	   0,
	   RowBytes + GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + 10,		// �E�B���h�E�̕�
	   Lines + GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + TITLE_HEIGHT + 29 + 10,		// �E�B���h�E�̍���
	   NULL,
	   NULL,
	   hInstance,
	   NULL
	   );

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  �֐�: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  �ړI:    ���C�� �E�B���h�E�̃��b�Z�[�W���������܂��B
//
//  WM_COMMAND	- �A�v���P�[�V���� ���j���[�̏���
//  WM_PAINT	- ���C�� �E�B���h�E�̕`��
//  WM_DESTROY	- ���~���b�Z�[�W��\�����Ė߂�
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	BYTE TxData[4] = { 0, 0, 0, 0 };
	DWORD BytesWritten;
	BOOL Shifted = FALSE;
	int boxes[] = { 503, 553, 603, -1 };
	unsigned short int x, y, Op_X, Op_Y;

	switch (message)
	{
	case WM_CREATE:
		// ���j���[�̃n���h�����擾�����ʂ̃p�����[�^�̂ݐݒ�
		hMenu = GetMenu(hWnd);
		menuInfo.cbSize = sizeof(MENUITEMINFO);

		// �R�����R���g���[���֌W�̏�����
		InitCommonControls();
		// �X�e�[�^�X�o�[�̍쐬
		hStatusBar = CreateStatusWindow( WS_CHILD | WS_VISIBLE | CCS_BOTTOM, _T(""), hWnd, ID_STATUS );
		// ��敪��
		SendMessage(hStatusBar, SB_SETPARTS, 3, (LPARAM)boxes);
		// �X�e�[�^�X�o�[�ɕ�����}��
		SendMessage(hStatusBar, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)_T("�E�B���h�E�����N���b�N���ă}�E�X���[�h"));
		menuInfo.fMask = MIIM_STATE;
		menuInfo.fState = MFS_CHECKED;
		switch (FPSmode)
		{
		case FPS60:
			SetMenuItemInfo(hMenu, IDM_60FPS, FALSE, &menuInfo);
			SendMessage(hStatusBar, SB_SETTEXT, 1 | SBT_NOBORDERS, (LPARAM)_T("60fps"));
			break;
		case FPS30:
			SetMenuItemInfo(hMenu, IDM_30FPS, FALSE, &menuInfo);
			SendMessage(hStatusBar, SB_SETTEXT, 1 | SBT_NOBORDERS, (LPARAM)_T("30fps"));
			break;
		case FPS15:
			SetMenuItemInfo(hMenu, IDM_15FPS, FALSE, &menuInfo);
			SendMessage(hStatusBar, SB_SETTEXT, 1 | SBT_NOBORDERS, (LPARAM)_T("15fps"));
			break;
		case FPS07:
			SetMenuItemInfo(hMenu, IDM_07FPS, FALSE, &menuInfo);
			SendMessage(hStatusBar, SB_SETTEXT, 1 | SBT_NOBORDERS, (LPARAM)_T("7.5fps"));
			break;
		default:
			break;
		}
		SendMessage(hStatusBar, SB_SETTEXT, 2 | SBT_NOBORDERS, (LPARAM)_T(""));
		SendMessage(hStatusBar, SB_SETTEXT, 3 | SBT_NOBORDERS, (LPARAM)_T(""));

		// ��V�O�i����ԂƂ��āC�C�x���g�I�u�W�F�N�g�𐶐�����
		hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		FT_SetEventNotification(ftHandle, FT_EVENT_RXCHAR, hEvent);

		// �X���b�h���쐬
		hRxThread = CreateThread(NULL, 0, GetUSB, NULL, 0, &ThRxID);

		// �ŏ��̃C�x���g�𔭍s
		SetEvent(hEvent);

		// ����J�n
		TxData[0] = 0xC1; TxData[1] = Offset_X2; TxData[2] = Offset_Y2;
		FT_Write(ftHandle, TxData, 4, &BytesWritten);
		TxData[0] = 0xC2; TxData[1] = Offset_X4; TxData[2] = Offset_Y4;
		FT_Write(ftHandle, TxData, 4, &BytesWritten);
		TxData[0] = 0xC0; TxData[1] = BOXmode | FPSmode;
		FT_Write(ftHandle, TxData, 4, &BytesWritten);
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		// �I�����ꂽ���j���[�̉��:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_ONOFF:
			menuInfo.fMask = MIIM_TYPE;
			menuInfo.fType = MFT_STRING;
			if (!BOXmode)
			{
				menuInfo.dwTypeData = _T("�ꎞ��~");
				BOXmode = 0x80;
			}
			else
			{
				menuInfo.dwTypeData = _T("�ĊJ");
				BOXmode = 0;
			}
			SetMenuItemInfo(hMenu, IDM_ONOFF, FALSE, &menuInfo);
			TxData[0] = 0xC0; TxData[1] = BOXmode | FPSmode;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			break;
		case IDM_60FPS:
			if (FPSmode == FPS60) break;
			MenuUnchecked(FPSmode);
			menuInfo.fState = MFS_CHECKED;
			FPSmode = FPS60;
			SetMenuItemInfo(hMenu, IDM_60FPS, FALSE, &menuInfo);
			TxData[0] = 0xC0; TxData[1] = BOXmode | FPS60;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			SendMessage(hStatusBar, SB_SETTEXT, 1 | SBT_NOBORDERS, (LPARAM)_T("60fps"));
			break;
		case IDM_30FPS:
			if (FPSmode == FPS30) break;
			MenuUnchecked(FPSmode);
			menuInfo.fState = MFS_CHECKED;
			FPSmode = FPS30;
			SetMenuItemInfo(hMenu, IDM_30FPS, FALSE, &menuInfo);
			TxData[0] = 0xC0; TxData[1] = BOXmode | FPS30;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			SendMessage(hStatusBar, SB_SETTEXT, 1 | SBT_NOBORDERS, (LPARAM)_T("30fps"));
			break;
		case IDM_15FPS:
			if (FPSmode == FPS15) break;
			MenuUnchecked(FPSmode);
			menuInfo.fState = MFS_CHECKED;
			FPSmode = FPS15;
			SetMenuItemInfo(hMenu, IDM_15FPS, FALSE, &menuInfo);
			TxData[0] = 0xC0; TxData[1] = BOXmode | FPS15;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			SendMessage(hStatusBar, SB_SETTEXT, 1 | SBT_NOBORDERS, (LPARAM)_T("15fps"));
			break;
		case IDM_07FPS:
			if (FPSmode == FPS07) break;
			MenuUnchecked(FPSmode);
			menuInfo.fState = MFS_CHECKED;
			FPSmode = FPS07;
			SetMenuItemInfo(hMenu, IDM_07FPS, FALSE, &menuInfo);
			TxData[0] = 0xC0; TxData[1] = BOXmode | FPS07;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			SendMessage(hStatusBar, SB_SETTEXT, 1 | SBT_NOBORDERS, (LPARAM)_T("7.5fps"));
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_KEYDOWN:
		if (GetKeyState(VK_SHIFT)&0x8000)
		{
			Shifted = TRUE;
		}
		else
		{
			Shifted = FALSE;
		}

		switch (wParam)
		{
		case VK_F1:
			TxData[0] = 0xC8;
			TxData[1] = 0x3F;
			if (Shifted)
			{
				TxData[2] = 0x76;
			}
			else
			{
				TxData[2] = 0x71;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_F2:
			TxData[0] = 0xC8;
			TxData[1] = 0x3F;
			if (Shifted)
			{
				TxData[2] = 0x77;
			}
			else
			{
				TxData[2] = 0x72;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_F3:
			TxData[0] = 0xC8;
			TxData[1] = 0x3F;
			if (Shifted)
			{
				TxData[2] = 0x78;
			}
			else
			{
				TxData[2] = 0x73;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_F4:
			TxData[0] = 0xC8;
			TxData[1] = 0x3F;
			if (Shifted)
			{
				TxData[2] = 0x79;
			}
			else
			{
				TxData[2] = 0x74;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_F5:
			TxData[0] = 0xC8;
			TxData[1] = 0x3F;
			if (Shifted)
			{
				TxData[2] = 0x7A;
			}
			else
			{
				TxData[2] = 0x75;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_LEFT:
			if (Shifted)
			{
				if (L200)
				{
					Offset_X2++;
					if (Offset_X2 > 50) Offset_X2 = 50;
					TxData[0] = 0xC1; TxData[1] = Offset_X2; TxData[2] = Offset_Y2;
				}
				else
				{
					Offset_X4++;
					if (Offset_X4 > 50) Offset_X4 = 50;
					TxData[0] = 0xC2; TxData[1] = Offset_X4; TxData[2] = Offset_Y4;
				}
			}
			else
			{
				TxData[0] = 0xC8; TxData[1] = 0xBF; TxData[2] = 0x1D;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_RIGHT:
			if (Shifted)
			{
				if (L200)
				{
					Offset_X2--;
					if (Offset_X2 < 10) Offset_X2 = 10;
					TxData[0] = 0xC1; TxData[1] = Offset_X2; TxData[2] = Offset_Y2;
				}
				else
				{
					Offset_X4--;
					if (Offset_X4 < 10) Offset_X4 = 10;
					TxData[0] = 0xC2; TxData[1] = Offset_X4; TxData[2] = Offset_Y4;
				}
			}
			else
			{
				TxData[0] = 0xC8; TxData[1] = 0xBF; TxData[2] = 0x1C;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_UP:
			if (Shifted)
			{
				if (L200)
				{
					Offset_Y2++;
					if (Offset_Y2 > 40) Offset_Y2 = 40;
					TxData[0] = 0xC1; TxData[1] = Offset_X2; TxData[2] = Offset_Y2;
				}
				else
				{
					Offset_Y4++;
					if (Offset_Y4 > 40) Offset_Y4 = 40;
					TxData[0] = 0xC2; TxData[1] = Offset_X4; TxData[2] = Offset_Y4;
				}
			}
			else
			{
				TxData[0] = 0xC8; TxData[1] = 0xBF; TxData[2] = 0x1E;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_DOWN:
			if (Shifted)
			{
				if (L200)
				{
					Offset_Y2--;
					if (Offset_Y2 < 10) Offset_Y2 = 10;
					TxData[0] = 0xC1; TxData[1] = Offset_X2; TxData[2] = Offset_Y2;
				}
				else
				{
					Offset_Y4--;
					if (Offset_Y4 < 10) Offset_Y4 = 10;
					TxData[0] = 0xC2; TxData[1] = Offset_X4; TxData[2] = Offset_Y4;
				}
			}
			else
			{
				TxData[0] = 0xC8; TxData[1] = 0xBF; TxData[2] = 0x1F;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_F11:
			TxData[0] = 0xC8;
			if (Shifted)
			{
				TxData[1] = 0x3D;
				TxData[2] = 0xC0;
			}
			else
			{
				TxData[1] = 0x3F;
				TxData[2] = 0xC1;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_F12:
			TxData[0] = 0xC8;
			if (Shifted)
			{
				TxData[1] = 0x3D;
				TxData[2] = 0xC5;
			}
			else
			{
				TxData[1] = 0x3F;
				TxData[2] = 0xC3;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_KANA:
			if (KNLocked)
			{
				KNLocked = FALSE;
				SendMessage(hStatusBar, SB_SETTEXT, 3 | SBT_NOBORDERS, (LPARAM)_T(""));
			}
			else
			{
				KNLocked = TRUE;
				SendMessage(hStatusBar, SB_SETTEXT, 3 | SBT_NOBORDERS, (LPARAM)_T("KANA"));
			}
			Pushed = TRUE;
			Released = TRUE;
			break;
		case VK_CAPITAL:
			if (GetKeyState(VK_CAPITAL) & 1)
			{
				SendMessage(hStatusBar, SB_SETTEXT, 2 | SBT_NOBORDERS, (LPARAM)_T("CAPS"));
			}
			else
			{
				SendMessage(hStatusBar, SB_SETTEXT, 2 | SBT_NOBORDERS, (LPARAM)_T(""));
			}
			Pushed = TRUE;
			Released = TRUE;
			break;
		case VK_NUMPAD0:
		case VK_NUMPAD1:
		case VK_NUMPAD2:
		case VK_NUMPAD3:
		case VK_NUMPAD4:
		case VK_NUMPAD5:
		case VK_NUMPAD6:
		case VK_NUMPAD7:
		case VK_NUMPAD8:
		case VK_NUMPAD9:
			TxData[0] = 0xC8;
			if (Shifted)
			{
				TxData[1] = 0x3D;
			}
			else
			{
				TxData[1] = 0x3F;
			}
			TxData[2] = (wParam & 0xFF) - 0x30;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		case VK_MULTIPLY:
		case VK_ADD:
		case VK_SUBTRACT:
		case VK_DECIMAL:
		case VK_DIVIDE:
			TxData[0] = 0xC8;
			if (Shifted)
			{
				TxData[1] = 0x3D;
			}
			else
			{
				TxData[1] = 0x3F;
			}
			TxData[2] = (wParam & 0xFF) - 0x40;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		default:
			break;
		}
		break;

	case WM_CHAR:
		if (Pushed)
		{
			Pushed = FALSE;
			break;
		}
		TxData[0] = 0xC8;
		if (KNLocked)
		{
			TxData[1] = 0xBB;
			TxData[2] = KNtable[(wchar_t)wParam];
		}
		else
		{
			TxData[1] = 0xBF;
			TxData[2] = (wchar_t)wParam & 0xFF;
		}
		FT_Write(ftHandle, TxData, 4, &BytesWritten);
		break;

	case WM_SYSKEYDOWN:
		if (GetKeyState(VK_SHIFT) & 0x8000)
		{
			Shifted = TRUE;
		}
		else
		{
			Shifted = FALSE;
		}

		switch (wParam)
		{
		case VK_F10:
			TxData[0] = 0xC8;
			if (Shifted)
			{
				TxData[1] = 0x3D;
				TxData[2] = 0xC6;
			}
			else
			{
				TxData[1] = 0x3F;
				TxData[2] = 0xC4;
			}
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
			Pushed = TRUE;
			break;
		default:
			break;
		}
		break;

	case WM_SYSCHAR:
		if (Pushed)
		{
			Pushed = FALSE;
			break;
		}
		if (wParam == VK_RETURN)
		{
			MouseOn = FALSE;
			SetCursorPos(mp.x, mp.y);
			ShowCursor(TRUE);
			SendMessage(hStatusBar, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)_T("�E�B���h�E�����N���b�N���ă}�E�X���[�h"));
			Released = TRUE;
			break;
		}
		TxData[0] = 0xC8; TxData[1] = 0xAF; TxData[2] = GRtable[(wchar_t)wParam];
		FT_Write(ftHandle, TxData, 4, &BytesWritten);
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (Released)
		{
			Released = FALSE;
			break;
		}
		TxData[0] = 0xC8; TxData[1] = 0xFF; TxData[2] = 0;
		FT_Write(ftHandle, TxData, 4, &BytesWritten);
		break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		if (!MouseOn)
		{
			MouseOn = TRUE;
			GetCursorPos(&mp);
			ShowCursor(FALSE);
			SendMessage(hStatusBar, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)_T("ALT+Enter�Ń}�E�X���[�h����"));
			// ��\����Ԃ̃}�E�X�J�[�\���𒆉��ɒu�����߂̍��W�擾
			cp.x = 325; cp.y = 205;
			ClientToScreen(hWnd, &cp);
			SetCursorPos(cp.x, cp.y);
		}
		else
		{
			LBTN = 0x01;
			TxData[0] = 0xC9; TxData[1] = RBTN + 0x01; TxData[2] = 0; TxData[3] = 0;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
		}
		break;

	case WM_LBUTTONUP:
		if (LBTN==0x01)
		{
			LBTN = 0;
			TxData[0] = 0xC9; TxData[1] = RBTN; TxData[2] = 0; TxData[3] = 0;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
		}
		break;

	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		if (MouseOn)
		{
			RBTN = 0x02;
			TxData[0] = 0xC9; TxData[1] = 0x02 + LBTN; TxData[2] = 0; TxData[3] = 0;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
		}
		break;

	case WM_RBUTTONUP:
		if (RBTN == 0x02)
		{
			RBTN = 0;
			TxData[0] = 0xC9; TxData[1] = LBTN; TxData[2] = 0; TxData[3] = 0;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
		}
		break;

	case WM_MOUSEMOVE:
		if (MouseOn)
		{
			x = LOWORD(lParam);
			y = HIWORD(lParam);
			if (x == 325 && y == 205)
			{
				break;
			}
			if (x >= 325)
			{
				x -= 325;
				Op_X = 0;
			}
			else
			{
				x = 325 - x;
				Op_X = 0x80;
			}
			if (y >= 205)
			{
				y -= 205;
				Op_Y = 0;
			}
			else
			{
				y = 205 - y;
				Op_Y = 0x40;
			}
			if (L200) y /= 2;
			SetCursorPos(cp.x, cp.y);
			TxData[0] = 0xC9; TxData[1] = Op_X + Op_Y + RBTN + LBTN; TxData[2] = x; TxData[3] = y;
			FT_Write(ftHandle, TxData, 4, &BytesWritten);
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// DIB�`��
		SetDIBitsToDevice(hdc, 0, 0, RowBytes, Lines, 0, 0, 0,
			Lines + TITLE_HEIGHT, g_lppxDIB, &g_biDIB, DIB_RGB_COLORS);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		TxData[0] = 0xC0; TxData[1] = FPSmode;
		FT_Write(ftHandle, TxData, 4, &BytesWritten);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// �o�[�W�������{�b�N�X�̃��b�Z�[�W �n���h���[�ł��B
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// USB ������
int InitUSB()
{
	FT_STATUS ftStatus;

	// USB ���I�[�v��
	ftStatus = FT_OpenEx("USBCONEMU", FT_OPEN_BY_DESCRIPTION, &ftHandle);
	if (ftStatus != FT_OK) return 1;

	// Output mode �ݒ�
	ftStatus = FT_SetBitMode(ftHandle, 0x00, 0x40);
	if (ftStatus != FT_OK) return 1;

	// �o�b�t�@�T�C�Y�ݒ�
	ftStatus = FT_SetUSBParameters(ftHandle, MAX_BUFSIZE, 0);
	if (ftStatus != FT_OK) return 1;

	// �f�o�C�X���Z�b�g
	ftStatus = FT_ResetDevice(ftHandle);
	if (ftStatus != FT_OK) return 1;

	// �o�b�t�@�N���A
	ftStatus = FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
	if (ftStatus != FT_OK) return 1;

	// Time Out �ݒ�
	ftStatus = FT_SetTimeouts(ftHandle, NULL, NULL);
	if (ftStatus != FT_OK) return 1;

	Sleep(150);
	return 0;
}

// USB �N���[�Y
void TrashUSB()
{
	FT_Close(ftHandle);
}

// USB ����f�[�^�𓾂�
// �ʃX���b�h�ɂĎ��s
DWORD CALLBACK GetUSB(void *param)
{
	BYTE buff[MAX_BUFSIZE];
	DWORD RxQueSize = 0;
	DWORD bytesRead = 0;
	FT_STATUS ftStatus = FT_OK;

	while (true)
	{
		//�I�u�W�F�N�g���V�O�i����ԂƂȂ�܂őҋ@
		WaitForSingleObject(hEvent, INFINITE);
		//ResetEvent(hEvent);

		FT_GetQueueStatus(ftHandle, &RxQueSize);
		while (RxQueSize)
		{
			ftStatus = FT_Read(ftHandle, buff, RxQueSize, &bytesRead);

			// �蔲�������O�o�b�t�@(�ǂݏo�����ɂ͔z������)
			if (wptr + bytesRead > MAX_RINGBUF)
			{
				memcpy(&RxBuff[wptr], buff, MAX_RINGBUF - wptr);
				memcpy(RxBuff, &buff[MAX_RINGBUF - wptr], bytesRead - (MAX_RINGBUF - wptr));
			}
			else
			{
				memcpy(&RxBuff[wptr], buff, bytesRead);
			}
			wptr += bytesRead;
			if (wptr >= MAX_RINGBUF)
			{
				wptr -= MAX_RINGBUF;
			}
			FT_GetQueueStatus(ftHandle, &RxQueSize);
		}
		//if (!RxQueSize) continue;
	}
	return 0;
}

// DIB�p���������ăA���P�[�g����
void reallocDIB()
{
	if (g_lppxDIB) HeapFree(GetProcessHeap(), 0, g_lppxDIB);

	// DIB�pBITMAPINFO���N���A
	ZeroMemory(&g_biDIB, sizeof(g_biDIB));

	// DIB�pBITMAPINFO�ݒ�
	g_biDIB.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	g_biDIB.bmiHeader.biWidth = RowBytes;
	g_biDIB.bmiHeader.biHeight = Lines;
	g_biDIB.bmiHeader.biPlanes = 1;
	g_biDIB.bmiHeader.biBitCount = 32;
	g_biDIB.bmiHeader.biCompression = BI_RGB;

	// DIB�p�s�N�Z����m��
	g_lppxDIB = (LPDWORD)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		RowBytes * Lines * 4);
}

// ��M�f�[�^��DIB�p�������ɓW�J
DWORD ConvPixel()
{
	int lnum;
	LPDWORD p, q;
	BYTE RGB;
	int R, G, B;

	// �t�b�^�ɏo��܂Ń����O�o�b�t�@����R�s�[
	do
	{
		if (rptr == MAX_RINGBUF) rptr = 0;	// ���E���h���b�v
		if (rptr == wptr) return 0;			// �����O�o�b�t�@�Ƀf�[�^�Ȃ�
		lbuf[bptr] = RxBuff[rptr++];
	} while ((lbuf[bptr++] & 0x80) != 0x80);

	L200 = (lbuf[bptr - 1] == 0x80);
	bptr -= bptr % 3;
	if (bptr < 3)
	{
		bptr = 0;
		return 0;
	}
	// �\���͈͔���
	lnum = (lbuf[0] << 7) + lbuf[1]; //log[lnum]++;
	if (L200)
	{
		if (lnum > 0 && lnum < 400)
		{
			p = ppos[lnum];
			q = ppos[lnum - 1];
			for (size_t bytes = 2; bytes < 281; bytes += 3)
			{
				R = lbuf[bytes] << 2; G = lbuf[bytes + 1] << 1; B = lbuf[bytes + 2];
				// bit0
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				*(q++) = pval[RGB];
				// bit1
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				*(q++) = pval[RGB];
				// bit2
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				*(q++) = pval[RGB];
				// bit3
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				*(q++) = pval[RGB];
				// bit4
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				*(q++) = pval[RGB];
				// bit5
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				*(q++) = pval[RGB];
				// bit6
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				*(p++) = pval[RGB];
				*(q++) = pval[RGB];
			}
			bptr = 0;
			return lnum;
		}
		else
		{
			bptr = 0;
			return 0;
		}
	}
	else
	{
		if (lnum < 400)
		{
			p = ppos[lnum];
			for (size_t bytes = 2; bytes < 281; bytes += 3)
			{
				R = lbuf[bytes] << 2; G = lbuf[bytes + 1] << 1; B = lbuf[bytes + 2];
				// bit0
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				// bit1
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				// bit2
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				// bit3
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				// bit4
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				// bit5
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				R >>= 1; G >>= 1; B >>= 1;
				*(p++) = pval[RGB];
				// bit6
				RGB = (R & 0x04) | (G & 0x02) | (B & 0x01);
				*(p++) = pval[RGB];
			}
			bptr = 0;
			return lnum;
		}
		else
		{
			bptr = 0;
			return 0;
		}
	}
}

//
void MenuUnchecked(BYTE FPSmode)
{
	menuInfo.fMask = MIIM_STATE;
	menuInfo.fState = MFS_UNCHECKED;
	switch (FPSmode)
	{
	case FPS60:	// IDM_60FPS
		SetMenuItemInfo(hMenu, IDM_60FPS, FALSE, &menuInfo);
		break;
	case FPS30:	// IDM_30FPS
		SetMenuItemInfo(hMenu, IDM_30FPS, FALSE, &menuInfo);
		break;
	case FPS15:	// IDM_15FPS
		SetMenuItemInfo(hMenu, IDM_15FPS, FALSE, &menuInfo);
		break;
	case FPS07:	// IDM_07FPS
		SetMenuItemInfo(hMenu, IDM_07FPS, FALSE, &menuInfo);
		break;
	default:
		break;
	}
}

// INI�t�@�C���̕ۑ�����쐬����
bool MakeIniFileName(TCHAR *ini_fname, TCHAR *ini_fullpath)
{
	TCHAR module[_MAX_PATH + 1];
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	bool bRet = false;

	// EXE�����݂Ńt���p�X���擾
	if (GetModuleFileName(NULL, module, sizeof(module)) != 0){
		_tsplitpath_s(module, drive, _countof(drive), dir, _countof(dir), NULL, 0, NULL, 0);

		// �t�@�C�������쐬
		wsprintf(ini_fullpath, _T("%s%s%s"), drive, dir, ini_fname);
		bRet = true;
	}

	return bRet;
}
