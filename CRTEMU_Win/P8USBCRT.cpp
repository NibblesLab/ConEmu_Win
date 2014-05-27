//
// PC-8001 USB CRT エミュレータ for Windows
//
//             copyleft 2009 Koichi Nishida
//            on Gnu General Public License
//

#include <windows.h>
#include <tchar.h>
#include "stdafx.h"
#include "ftd2xx.h"
#include "resource.h"

// USB の最大バッファサイズ
#define MAX_BUFSIZE 256

// ウィンドウのタイトルの高さ
#define TITLE_HEIGHT (GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYMENU))

#define WINDOW_X 100				//ウィンドウの位置
#define WINDOW_Y 100

// タイマー
#define TIMER_ID     (100)			// タイマーのID
#define TIMER_ELAPSE (1000/60)		// WM_TIMERの発生間隔

// インスタンス
HINSTANCE hinst;

// ウィンドウ
HWND hwnd;

// USB のバッファサイズ
int Bufsize = 32;

// ROW バイト
int RowBytes = 416;

// ライン数
int Lines = 262;

// ワイドモード
int Wide = 1;

// USB ハンドル
FT_HANDLE ftHandle;

// DIB用変数
BITMAPINFO g_biDIB;
LPDWORD g_lppxDIB = NULL;

// フレームバッファ
BYTE* g_picbuf;

// プロトタイプ宣言
HWND CreateWin(HINSTANCE hInst);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL CALLBACK DlgProc(HWND hDlg, UINT mst, WPARAM wp, LPARAM lp);
int InitUSB();
void TrashUSB();
int GetUSB(BYTE *);
bool MakeIniFileName(char *ini_fname, char *ini_fullpath);

// DIB用メモリを再アロケートする
void reallocDIB()
{
	if (g_lppxDIB) HeapFree(GetProcessHeap(), 0, g_lppxDIB);

	// DIB用BITMAPINFOをクリア
	ZeroMemory(&g_biDIB, sizeof(g_biDIB));

	// DIB用BITMAPINFO設定
	g_biDIB.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	g_biDIB.bmiHeader.biWidth = RowBytes*(Wide+1);
	g_biDIB.bmiHeader.biHeight = Lines*(Wide+1);
	g_biDIB.bmiHeader.biPlanes = 1;
	g_biDIB.bmiHeader.biBitCount = 32;
	g_biDIB.bmiHeader.biCompression = BI_RGB;

	// DIB用ピクセル列確保
	g_lppxDIB = (LPDWORD)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		RowBytes*(Wide+1) * Lines*(Wide+1) * 4);
}

// メイン
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR pCmdLine, int showCmd)
{
	MSG msg;
	char ini_fname[256];

	hinst = hInst;

	// ini ファイルから設定を読む
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

	// USB 初期化
	if (InitUSB()) {
		MessageBox(NULL, _T("USB の初期化に失敗しました"), _T("エラー"), MB_OK);
		return 1;
	}

	// フレームバッファ確保
	g_picbuf = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x20000);

	reallocDIB();

	// ウィンドウを作成する
	hwnd = CreateWin(hInst);
	if(hwnd == NULL)
	{
		MessageBox(NULL, _T("ウィンドウの作成に失敗しました"), _T("エラー"), MB_OK);
		return 1;
	}

	// ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// タイマを作成する
	SetTimer(hwnd, TIMER_ID, TIMER_ELAPSE, NULL);

	// メッセージループ
	while (1) {
		BOOL ret = GetMessage(&msg, NULL, 0, 0);  // メッセージを取得する
		if (ret == 0 || ret == -1)
		{
			// アプリケーション終了メッセージが来ていたら、
			// あるいは GetMessage() が失敗したら
			break;
		}
		else
		{
			// メッセージを処理
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// タイマを破棄
	KillTimer(hwnd, TIMER_ID);

	// USB 解放
	TrashUSB();

	// メモリ開放
	HeapFree(GetProcessHeap(), 0, g_lppxDIB);
	HeapFree(GetProcessHeap(), 0, g_picbuf);

	// ini ファイルに設定を書き込み
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

// ウィンドウを作成する
HWND CreateWin(HINSTANCE hInst)
{
	WNDCLASSEX wc;

	// ウィンドウクラスの情報を設定
	wc.cbSize = sizeof(wc);					// 構造体サイズ
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;	// スタイル
	wc.lpfnWndProc = WndProc;				// ウィンドウプロシージャ
	wc.cbClsExtra = 0;						// 拡張情報１
	wc.cbWndExtra = 0;						// 拡張情報２
	wc.hInstance = hInst;					// インスタンスハンドル
	wc.hIcon = (HICON)LoadImage(			// アイコン
		NULL, MAKEINTRESOURCE(IDI_APPLICATION), IMAGE_ICON,
		0, 0, LR_DEFAULTSIZE | LR_SHARED
	);
	wc.hIconSm = wc.hIcon;					// 子アイコン
	wc.hCursor = (HCURSOR)LoadImage(		// マウスカーソル
		NULL, MAKEINTRESOURCE(IDC_ARROW), IMAGE_CURSOR,
		0, 0, LR_DEFAULTSIZE | LR_SHARED
	);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); // ウィンドウ背景
	wc.lpszMenuName = NULL;					// メニュー名
	wc.lpszClassName = _T("P8USBCRT");		// ウィンドウクラス名
	
	// ウィンドウクラスを登録する
	if( RegisterClassEx( &wc ) == 0 ){ return NULL; }

	// ウィンドウを作成する
	return CreateWindow(
		wc.lpszClassName,		// ウィンドウクラス名
		_T("P8USBCRT"),			// タイトルバーに表示する文字列
		WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,   // ウィンドウの種類
		WINDOW_X,				// ウィンドウを表示する位置（X座標）
		WINDOW_Y,				// ウィンドウを表示する位置（Y座標）
		RowBytes*(Wide+1),		// ウィンドウの幅
		Lines*(Wide+1) + TITLE_HEIGHT, // ウィンドウの高さ
		NULL,					// 親ウィンドウのウィンドウハンドル
		NULL,					// メニューハンドル
		hInst,					// インスタンスハンドル
		NULL					// その他の作成データ
	);
}

// Window をリサイズする
void ResizeWin()
{
	int cx = RowBytes*(Wide+1), cy = Lines*(Wide+1) + TITLE_HEIGHT;
	SetWindowPos(hwnd, NULL, 0, 0, cx, cy, SWP_NOMOVE);
}

// ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	HDC hdc;
	PAINTSTRUCT ps;
	unsigned char prev, dt = 0, length, val;
	static int bytes = 0;			// 10 フレーム分のバイト数 
	static int tcount = 0;			// タイトル更新用カウンタ
	static int tick = 0;			// フレーム更新の tick 数
	int i, x, y, ipic, adr;
	LPDWORD p, q, r;
	static HMENU hmenu;				// メニュー

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
	case WM_TIMER:	// タイマーメッセージ
		if(wp != TIMER_ID)
		{
			break;	// 識別IDが一致しないタイマーメッセージはDefWindowProc()に
		}
		// ヘッダ待ち
		do {
			prev = dt;
			if (GetUSB(&dt)) {
				// USB が抜けていたら終わる
				PostQuitMessage(0);
				return 1;
			}
			bytes++;
		} while (!((prev == 0 || prev == 1) && dt == 0x82));
		// ヘッダが	01 82	ならば I ピクチャ
		//			00 82	ならば P ピクチャ
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
					// 値と長さを得る
					if (GetUSB(&val)) {
						PostQuitMessage(0);
						return 1;
					}
					if (GetUSB(&length)) {
						PostQuitMessage(0);
						return 1;
					}
					bytes += 2;
					if ((val == 0x01) && (length == 0x82)) { // ヘッダを検出したら。。。
						ipic = 1;
						goto RESET;
					}
				}
				// I ピクチャなら、値をそのまま使う
				// P ピクチャなら、前の値に足す
				if (ipic) dt = val;
				else dt = g_picbuf[adr] + val;
				g_picbuf[adr++] = dt;
				length--;
				if (ipic || (val != 0)) {
					// 通常モード
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
		// タイトル表示
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
	case WM_PAINT: {	// 無効領域があるとき
			hdc = BeginPaint(hwnd, &ps);
			// DIB描画
			SetDIBitsToDevice(hdc, 0, 0, RowBytes*(Wide+1), Lines*(Wide+1), 0, 0, 0,
					Lines*(Wide+1)+TITLE_HEIGHT, g_lppxDIB, &g_biDIB, DIB_RGB_COLORS);
			EndPaint(hwnd, &ps);
		}
		return 0;
	case WM_CLOSE:
		SetMenu(hwnd, NULL);
		hmenu = NULL;
		break;

	case WM_DESTROY:        // ウィンドウが破棄されるとき
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}

// USB 初期化
int InitUSB()
{
	FT_STATUS ftStatus;
	
	// USB をオープン
	ftStatus = FT_OpenEx("P8USBCRT", FT_OPEN_BY_DESCRIPTION, &ftHandle);
	if (ftStatus != FT_OK) return 1;

	// Output mode 設定
	ftStatus = FT_SetBitMode(ftHandle, 0x00, 0x00);
	if (ftStatus != FT_OK) return 1;
	
	// バッファサイズ設定
	ftStatus = FT_SetUSBParameters(ftHandle, Bufsize, 0);
	if (ftStatus != FT_OK) return 1;

	// Time Out 設定
	ftStatus = FT_SetTimeouts(ftHandle, 3000, 3000);
	if (ftStatus != FT_OK) return 1;

	return 0;
}

// USB クローズ
void TrashUSB()
{
	FT_Close(ftHandle);
}

// USB からデータを得る
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

// ダイアログ用コールバック
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
							// バッファサイズ設定
							// 再オープンしないと駄目みたい
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

// INIファイルの保存先を作成する
bool MakeIniFileName(char *ini_fname, char *ini_fullpath)
{
    char module[_MAX_PATH + 1]; 
    char drive[_MAX_DRIVE]; 
    char dir[_MAX_DIR]; 
    bool bRet = false;

    // EXE名込みでフルパスを取得
    if (GetModuleFileName(NULL, module, sizeof(module)) != 0){
        _splitpath(module, drive, dir, NULL, NULL);

        // ファイル名を作成
        wsprintf(ini_fullpath, "%s%s%s", drive, dir, ini_fname); 
        bRet = true;
    }
    
    return bRet;
}

