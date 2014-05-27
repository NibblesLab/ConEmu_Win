//
// PC-8001 USB CRT �G�~�����[�^ for Windows
//
//             copyleft 2009 Koichi Nishida
//            on Gnu General Public License
//

#include <windows.h>
#include <tchar.h>
#include "stdafx.h"
#include "ftd2xx.h"
#include "resource.h"

// USB �̍ő�o�b�t�@�T�C�Y
#define MAX_BUFSIZE 256

// �E�B���h�E�̃^�C�g���̍���
#define TITLE_HEIGHT (GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU))

#define WINDOW_X 100				//�E�B���h�E�̈ʒu
#define WINDOW_Y 100

// �^�C�}�[
#define TIMER_ID     (100)			// �^�C�}�[��ID
#define TIMER_ELAPSE (1000/60)		// WM_TIMER�̔����Ԋu

// �C���X�^���X
HINSTANCE hinst;

// �E�B���h�E
HWND hwnd;

// USB �̃o�b�t�@�T�C�Y
int Bufsize = 32;

// ROW �o�C�g
int RowBytes = 416;

// ���C����
int Lines = 262;

// ���C�h���[�h
int Wide = 1;

// USB �n���h��
FT_HANDLE ftHandle;

// DIB�p�ϐ�
BITMAPINFO g_biDIB;
LPDWORD g_lppxDIB = NULL;

// �t���[���o�b�t�@
BYTE* g_picbuf;

// �v���g�^�C�v�錾
HWND CreateWin(HINSTANCE hInst);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL CALLBACK DlgProc(HWND hDlg, UINT mst, WPARAM wp, LPARAM lp);
int InitUSB();
void TrashUSB();
int GetUSB(BYTE *);
bool MakeIniFileName(char *ini_fname, char *ini_fullpath);

// DIB�p���������ăA���P�[�g����
void reallocDIB()
{
	if (g_lppxDIB) HeapFree(GetProcessHeap(), 0, g_lppxDIB);

	// DIB�pBITMAPINFO���N���A
	ZeroMemory(&g_biDIB, sizeof(g_biDIB));

	// DIB�pBITMAPINFO�ݒ�
	g_biDIB.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	g_biDIB.bmiHeader.biWidth = RowBytes*(Wide+1);
	g_biDIB.bmiHeader.biHeight = Lines*(Wide+1);
	g_biDIB.bmiHeader.biPlanes = 1;
	g_biDIB.bmiHeader.biBitCount = 32;
	g_biDIB.bmiHeader.biCompression = BI_RGB;

	// DIB�p�s�N�Z����m��
	g_lppxDIB = (LPDWORD)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		RowBytes*(Wide+1) * Lines*(Wide+1) * 4);
}

// ���C��
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine, int showCmd)
{
	MSG msg;
	char ini_fname[256];

	hinst = hInst;

	// ini �t�@�C������ݒ��ǂ�
	if (MakeIniFileName("P8USBCRT.ini", ini_fname)) {
		int value;
	    value = GetPrivateProfileInt("section", "wide", -1, ini_fname);
		if (value != -1) Wide = value;
	    value = GetPrivateProfileInt("section", "bufsize", -1, ini_fname);
		if (value != -1) Bufsize = value;
		 value = GetPrivateProfileInt("section", "rowbytes", -1, ini_fname);
		if (value != -1) RowBytes = value;
		 value = GetPrivateProfileInt("section", "lines", -1, ini_fname);
		if (value != -1) Lines = value;
	}

	// USB ������
	if (InitUSB()) {
		MessageBox(NULL, _T("USB �̏������Ɏ��s���܂���"), _T("�G���["), MB_OK);
		return 1;
	}

	// �t���[���o�b�t�@�m��
	g_picbuf = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x20000);

	reallocDIB();

	// �E�B���h�E���쐬����
	hwnd = CreateWin(hInst);
	if(hwnd == NULL)
	{
		MessageBox(NULL, _T("�E�B���h�E�̍쐬�Ɏ��s���܂���"), _T("�G���["), MB_OK);
		return 1;
	}

	// �E�B���h�E��\������
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// �^�C�}���쐬����
	SetTimer(hwnd, TIMER_ID, TIMER_ELAPSE, NULL);

	// ���b�Z�[�W���[�v
	while (1) {
		BOOL ret = GetMessage(&msg, NULL, 0, 0);  // ���b�Z�[�W���擾����
		if (ret == 0 || ret == -1)
		{
			// �A�v���P�[�V�����I�����b�Z�[�W�����Ă�����A
			// ���邢�� GetMessage() �����s������
			break;
		}
		else
		{
			// ���b�Z�[�W������
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// �^�C�}��j��
	KillTimer(hwnd, TIMER_ID);

	// USB ���
	TrashUSB();

	// �������J��
	HeapFree(GetProcessHeap(), 0, g_lppxDIB);
	HeapFree(GetProcessHeap(), 0, g_picbuf);

	// ini �t�@�C���ɐݒ����������
	if (MakeIniFileName("P8USBCRT.ini", ini_fname)) {
		char cstr[10];
		wsprintf((LPSTR)cstr, _T("%d"), Wide);
		WritePrivateProfileString( "section", "wide", cstr, ini_fname);
		wsprintf((LPSTR)cstr, _T("%d"), Bufsize);
		WritePrivateProfileString( "section", "bufsize", cstr, ini_fname);
		wsprintf((LPSTR)cstr, _T("%d"), RowBytes);
		WritePrivateProfileString( "section", "rowbytes", cstr, ini_fname);
		wsprintf((LPSTR)cstr, _T("%d"), Lines);
		WritePrivateProfileString( "section", "lines", cstr, ini_fname);
	}
	return 0;
}

// �E�B���h�E���쐬����
HWND CreateWin(HINSTANCE hInst)
{
	WNDCLASSEX wc;

	// �E�B���h�E�N���X�̏���ݒ�
	wc.cbSize = sizeof(wc);					// �\���̃T�C�Y
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;	// �X�^�C��
	wc.lpfnWndProc = WndProc;				// �E�B���h�E�v���V�[�W��
	wc.cbClsExtra = 0;						// �g�����P
	wc.cbWndExtra = 0;						// �g�����Q
	wc.hInstance = hInst;					// �C���X�^���X�n���h��
	wc.hIcon = (HICON)LoadImage(			// �A�C�R��
		NULL, MAKEINTRESOURCE(IDI_APPLICATION), IMAGE_ICON,
		0, 0, LR_DEFAULTSIZE | LR_SHARED
	);
	wc.hIconSm = wc.hIcon;					// �q�A�C�R��
	wc.hCursor = (HCURSOR)LoadImage(		// �}�E�X�J�[�\��
		NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR,
		0, 0, LR_DEFAULTSIZE | LR_SHARED
	);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // �E�B���h�E�w�i
	wc.lpszMenuName = NULL;					// ���j���[��
	wc.lpszClassName = _T("P8USBCRT");		// �E�B���h�E�N���X��
	
	// �E�B���h�E�N���X��o�^����
	if( RegisterClassEx( &wc ) == 0 ){ return NULL; }

	// �E�B���h�E���쐬����
	return CreateWindow(
		wc.lpszClassName,		// �E�B���h�E�N���X��
		_T("P8USBCRT"),			// �^�C�g���o�[�ɕ\�����镶����
		WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,   // �E�B���h�E�̎��
		WINDOW_X,				// �E�B���h�E��\������ʒu�iX���W�j
		WINDOW_Y,				// �E�B���h�E��\������ʒu�iY���W�j
		RowBytes*(Wide+1),		// �E�B���h�E�̕�
		Lines*(Wide+1) + TITLE_HEIGHT, // �E�B���h�E�̍���
		NULL,					// �e�E�B���h�E�̃E�B���h�E�n���h��
		NULL,					// ���j���[�n���h��
		hInst,					// �C���X�^���X�n���h��
		NULL					// ���̑��̍쐬�f�[�^
	);
}

// Window �����T�C�Y����
void ResizeWin()
{
	int cx = RowBytes*(Wide+1), cy = Lines*(Wide+1) + TITLE_HEIGHT;
	SetWindowPos(hwnd, NULL, 0, 0, cx, cy, SWP_NOMOVE);
}

// �E�B���h�E�v���V�[�W��
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	HDC hdc;
	PAINTSTRUCT ps;
	unsigned char prev, dt = 0, length, val;
	static int bytes = 0;			// 10 �t���[�����̃o�C�g�� 
	static int tcount = 0;			// �^�C�g���X�V�p�J�E���^
	static int tick = 0;			// �t���[���X�V�� tick ��
	int i, x, y, ipic, adr;
	LPDWORD p, q, r;
	static HMENU hmenu;				// ���j���[

	switch (msg) {
	case WM_CREATE:
		hmenu = LoadMenu(NULL, _T("IDR_MENU"));
		SetMenu(hwnd, hmenu);
		CheckMenuItem(hmenu, ID_WIDE, Wide ? MF_CHECKED : MF_UNCHECKED);
		DrawMenuBar(hwnd);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wp)) {
		case ID_EXIT:
			DestroyWindow(hwnd);
			break;
		case ID_WIDE: {
			Wide = !Wide;
			CheckMenuItem(hmenu, ID_WIDE, Wide ? MF_CHECKED : MF_UNCHECKED);
			DrawMenuBar(hwnd);
			ResizeWin();
			return 0;
		}
		case ID_PREF: {
			DialogBox(hinst, "IDR_DIALOG", hwnd, (DLGPROC)DlgProc);
			break;
		}
		}
		break;
	case WM_TIMER:	// �^�C�}�[���b�Z�[�W
		if(wp != TIMER_ID)
		{
			break;	// ����ID����v���Ȃ��^�C�}�[���b�Z�[�W��DefWindowProc()��
		}
		// �w�b�_�҂�
		do {
			prev = dt;
			if (GetUSB(&dt)) {
				// USB �������Ă�����I���
				PostQuitMessage(0);
				return 1;
			}
			bytes++;
		} while (!((prev == 0 || prev == 1) && dt == 0x82));
		// �w�b�_��	01 82	�Ȃ�� I �s�N�`��
		//			00 82	�Ȃ�� P �s�N�`��
		ipic = prev;
RESET:
		dt = 0; val = 0; length = 0; adr = 0;
		p = g_lppxDIB + RowBytes*(Wide+1) * (Lines*(Wide+1)-1);
		for (y = 0; y < Lines; y++) {
			q = p;
			r = q - RowBytes*(Wide+1);
			for (x = 0; x < RowBytes; x++) {
				BYTE R, G, B;
				DWORD pval;
				if (length == 0) {
					// �l�ƒ����𓾂�
					if (GetUSB(&val)) {
						PostQuitMessage(0);
						return 1;
					}
					if (GetUSB(&length)) {
						PostQuitMessage(0);
						return 1;
					}
					bytes += 2;
					if ((val == 0x01) && (length == 0x82)) { // �w�b�_�����o������B�B�B
						ipic = 1;
						goto RESET;
					}
				}
				// I �s�N�`���Ȃ�A�l�����̂܂܎g��
				// P �s�N�`���Ȃ�A�O�̒l�ɑ���
				if (ipic) dt = val;
				else dt = g_picbuf[adr] + val;
				g_picbuf[adr++] = dt;
				length--;
				if (ipic || (val != 0)) {
					// �ʏ탂�[�h
					for (i=0; i<2; i++) {
						R = (dt&0x1)*255;
						G = ((dt>>1)&1)*255;
						B = ((dt>>2)&1)*255;
						pval = ((R << 16) | (G << 8) | B);
						if (Wide || (i==0)) *(p++) = pval;
						if (Wide) *(r++) = pval; 
						dt >>= 4;
					}
				} else {
					p += (Wide ? 2 : 1); r += (Wide ? 2 : 1);
				}
			}
			p = q - (RowBytes*(Wide+1)) * (Wide ? 2 : 1);
		}
		// �^�C�g���\��
		{
			int ticks = GetTickCount()-tick;
			tick = GetTickCount();
			if (++tcount == 10) {
				char cstr[128];
				wsprintf((LPSTR)cstr, _T("P8USBCRT [%d frames/sec] [%d bytes/frame]"), 1000/ticks, bytes/10);
				SetWindowText(hwnd, (LPCSTR)cstr);
				bytes = 0;
				tcount = 0;
			}
		}
		InvalidateRect( hwnd, NULL, FALSE );
		return 0;
	case WM_PAINT: {	// �����̈悪����Ƃ�
			hdc = BeginPaint(hwnd, &ps);
			// DIB�`��
			SetDIBitsToDevice(hdc, 0, 0, RowBytes*(Wide+1), Lines*(Wide+1), 0, 0, 0,
					Lines*(Wide+1)+TITLE_HEIGHT, g_lppxDIB, &g_biDIB, DIB_RGB_COLORS);
			EndPaint(hwnd, &ps);
		}
		return 0;
	case WM_CLOSE:
		SetMenu(hwnd, NULL);
		hmenu = NULL;
		break;

	case WM_DESTROY:        // �E�B���h�E���j�������Ƃ�
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}

// USB ������
int InitUSB()
{
	FT_STATUS ftStatus;
	
	// USB ���I�[�v��
	ftStatus = FT_OpenEx("P8USBCRT", FT_OPEN_BY_DESCRIPTION, &ftHandle);
	if (ftStatus != FT_OK) return 1;

	// Output mode �ݒ�
	ftStatus = FT_SetBitMode(ftHandle, 0x00, 0x00);
	if (ftStatus != FT_OK) return 1;
	
	// �o�b�t�@�T�C�Y�ݒ�
	ftStatus = FT_SetUSBParameters(ftHandle, Bufsize, 0);
	if (ftStatus != FT_OK) return 1;

	// Time Out �ݒ�
	ftStatus = FT_SetTimeouts(ftHandle, 3000, 3000);
	if (ftStatus != FT_OK) return 1;

	return 0;
}

// USB �N���[�Y
void TrashUSB()
{
	FT_Close(ftHandle);
}

// USB ����f�[�^�𓾂�
int GetUSB(BYTE *dt)
{
	static unsigned char buff[MAX_BUFSIZE];
	static unsigned long bytesRead = 0;
	static unsigned int ptr = 0;
	FT_STATUS ftStatus = FT_OK;

	if (ptr == 0) {
		ftStatus = FT_Read(ftHandle, buff, Bufsize, &bytesRead);
		if (bytesRead == 0 || ftStatus != FT_OK) return 1;
	}
	*dt = buff[ptr];
	if (++ptr == bytesRead) ptr = 0;
	return 0;
}

// �_�C�A���O�p�R�[���o�b�N
BOOL CALLBACK DlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp)
{
	char cstr[64];
	switch (msg) {
		case WM_INITDIALOG:
			wsprintf((LPSTR)cstr, _T("%d"), Bufsize);
			SetDlgItemText(hdlg, ID_BUFSZ, cstr);
			wsprintf((LPSTR)cstr, _T("%d"), RowBytes);
			SetDlgItemText(hdlg, ID_ROWB, cstr);
			wsprintf((LPSTR)cstr, _T("%d"), Lines);
			SetDlgItemText(hdlg, ID_LINES, cstr);
			{
				HWND hcheck = GetDlgItem(hdlg, ID_WIDE_CHECK);
				SendMessage(hcheck, BM_SETCHECK,(WPARAM)(Wide?BST_CHECKED:BST_UNCHECKED),0);
			}
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wp)) {
				case ID_OK:
					if (GetDlgItemText(hdlg, ID_BUFSZ, cstr, 63)) {
						int tmp = atoi(cstr);
						if (0 < tmp && tmp <= MAX_BUFSIZE) {
							Bufsize = tmp;
							// �o�b�t�@�T�C�Y�ݒ�
							// �ăI�[�v�����Ȃ��Ƒʖڂ݂���
							TrashUSB();
							InitUSB();
							FT_SetUSBParameters(ftHandle, Bufsize, 0);
						}	
					}
					if (GetDlgItemText(hdlg, ID_ROWB, cstr, 63)) {
						RowBytes = atoi(cstr);
					}
					if (GetDlgItemText(hdlg, ID_LINES, cstr, 63)) {
						Lines = atoi(cstr);
					}
					{
						HWND hcheck = GetDlgItem(hdlg, ID_WIDE_CHECK);
						Wide = ((int)SendMessage(hcheck, BM_GETCHECK, 0, 0)) == BST_CHECKED ? 1 : 0;
					}
					reallocDIB();
					EndDialog(hdlg, ID_OK);
					ResizeWin();
					return TRUE;
				case ID_CANCEL:
					EndDialog(hdlg, ID_CANCEL);
					return TRUE;
				default:
					return FALSE;
			}
		case WM_CLOSE:
			EndDialog(hdlg, ID_CANCEL);
			return TRUE;
	}
	return FALSE;
}

// INI�t�@�C���̕ۑ�����쐬����
bool MakeIniFileName(char *ini_fname, char *ini_fullpath)
{
    char module[_MAX_PATH + 1]; 
    char drive[_MAX_DRIVE]; 
    char dir[_MAX_DIR]; 
    bool bRet = false;

    // EXE�����݂Ńt���p�X���擾
    if (GetModuleFileName(NULL, module, sizeof(module)) != 0){
        _splitpath(module, drive, dir, NULL, NULL);

        // �t�@�C�������쐬
        wsprintf(ini_fullpath, "%s%s%s", drive, dir, ini_fname); 
        bRet = true;
    }
    
    return bRet;
}

