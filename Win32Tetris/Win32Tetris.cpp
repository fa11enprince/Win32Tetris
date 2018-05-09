// @author  fa11enprince
// 参考
// Tetris :
//    [テトリスの作り方]
//       http://www13.plala.or.jp/kymats/study/game_other/SPACE_TETRIS/st1.html
//
// GDI :
//    [BMPファイルを使った簡単なパラパラアニメーション]
//       http://www13.plala.or.jp/kymats/study/MULTIMEDIA/bmp_animation.html
//    [フォントの種類]
//       http://wisdom.sakura.ne.jp/system/winapi/win32/win131.html
//    [その他MSDN]
//       http://msdn.microsoft.com/ja-jp/library/cc428720.aspx
//       http://msdn.microsoft.com/ja-jp/library/cc410411.aspx
//
// Multi Thread:
//    [MSDN]
//       http://msdn.microsoft.com/ja-jp/library/kdzttdcb.aspx
//    [Win32のマルチスレッドで遊ぶための備忘録]
//       http://www.geocities.jp/debu0510/personal/kusosure.html#m2
//
// Windows API:
//    『猫でもわかるWindowsプログラミング 第4版』
//
// WM_APP
//       http://chokuto.ifdef.jp/advanced/message/WM_APP.html
// WndProcクラス化
//       https://qiita.com/seekerkrt/items/2478c05c590c308146fd
//       https://msdn.microsoft.com/ja-jp/library/windows/desktop/ff381400(v=vs.85).aspx
//       
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#include "Resource.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <tchar.h>
#include <ctime>
#include <process.h>
#pragma comment(lib, "winmm.lib")

#define _DEBUG
#ifdef _DEBUG
#   define MyOutputDebugString( str, ... ) \
      { \
        TCHAR c[256]; \
        _stprintf( c, _T(str), __VA_ARGS__ ); \
        OutputDebugString( c ); \
      }
#else
#    define MyOutputDebugString( str, ... ) // 空実装
#endif

#define MAX_LOADSTRING 100
// アプリケーション固有のメッセージ定義
#define WM_MUTEX       WM_APP
#define WM_RESTART     (WM_APP + 1)
#define MUTEX_NAME     _T("TetrisMutex")

namespace {
	const int PieceWidth = 4;
	const int PieceHeight = 4;
	const int FieldWidth = 14;
	const int FieldHeight = 24;
	const int SizeCellPixelX = 20;
	const int SizeCellPixelY = 20;
	const int MovePixcel = 20;
	const int SizeBmPixelX = 420;
	const int SizeBmPixelY = 520;

}

class Point {
public:
	int x;
	int y;
};

class Cell {
private:
	int block_;
	int color_;
public:
	void clear() {
		block_ = 0;
		color_ = 0;
	}
	void create(const int color) {
		block_ = 1;
		color_ = color;
	}
	int getBlock() const {
		return block_;
	}
	int getColor() const {
		return color_;
	}
};


class Field
{
public:
	Field()
	{
		clear();
	}

	void clear()
	{
		for (int y = 0; y < FieldHeight; y++)
		{
			for (int x = 0; x < FieldWidth; x++)
			{
				cells_[x][y].clear();
			}
		}
	}

	void fixPiece(const Point pos, const Cell cells[][PieceHeight])
	{
		for (int y = 0; y < PieceHeight; y++)
		{
			for (int x = 0; x < PieceWidth; x++)
			{
MyOutputDebugString("%d\n", cells[x][y].getBlock());
MyOutputDebugString("%d\n", pos.y);
				// 上に飛び出てなければ固定
				if (cells[x][y].getBlock() && pos.y + y >= 0)
				{
					cells_[pos.x + x][pos.y + y] = cells[x][y];
				}
			}
		}
	}

	int deleteLine()
	{
		int delCount = 0;
		for (int y = FieldHeight - 1; y >= 0; y--)
		{
			int xNumPerLine = 0;
			for (int x = 0; x < FieldWidth; x++)
			{
				if (cells_[x][y].getBlock())
				{
					xNumPerLine++;
				}
			}
			if (xNumPerLine != FieldWidth)
			{
				continue;
			}
			delCount++;
			for (int x = 0; x < FieldWidth; x++)
			{
				cells_[x][y].clear();
			}
		}
		return delCount;
	}

	void shiftLine(int delCount)
	{
		// 下から上に走査
		for (int y = FieldHeight - 1; y >= 0 && delCount > 0;)
		{
			int xNumPerLine = 0;
			for (int x = 0; x < FieldWidth; x++)
			{
				if (cells_[x][y].getBlock())
				{
					xNumPerLine++;
				}
			}
			// 行がそろってない そろっていればこの時点で0になる
			if (xNumPerLine != 0)
			{
				y--;       // そろってないときだけ上に
				continue;
			}
			
			for (int ty = y; ty >= 0; ty--)
			{
				for (int tx = 0; tx < FieldWidth; tx++)
				{
					if (ty - 1 >= 0)
					{
						cells_[tx][ty] = cells_[tx][ty - 1];
					}
					// 0行目より上の場合
					else
					{
						cells_[tx][0].clear();
					}
				}
			}
			// 一行詰める
			delCount--;
		}
	}

	bool isStacked() const
	{
		if (cells_[6][0].getBlock() || cells_[7][0].getBlock())
		{
			return true;
		}
		return false;
	}

	Cell getCell(int x, int y) const
	{
		return cells_[x][y];
	}

private:
	Cell cells_[FieldWidth][FieldHeight];
};

class Piece
{
public:
	enum Dir
	{
		Left,
		Right,
		Down,
	};

	Piece(Field *field)
	{
		clear();
		initPos();
		field_ = field;
	}

	virtual ~Piece() { }

	void clear()
	{
		for (int y = 0; y < PieceHeight; y++)
		{
			for (int x = 0; x < PieceWidth; x++)
			{
				cells_[x][y].clear();
			}
		}
	}

public:
	bool move(Dir dir)
	{
		switch (dir)
		{
		case Left:
		{
			Point pos = { pos_.x - 1, pos_.y };
			if (isMovable(pos, cells_))
			{
				pos_ = pos;
			}
			else
			{
				return false;
			}
			break;
		}
		case Right:
		{
			Point pos = { pos_.x + 1, pos_.y };
			if (isMovable(pos, cells_))
			{
				pos_ = pos;
			}
			else
			{
				return false;
			}
			break;
		}
		case Down:
		{
			Point pos = { pos_.x, pos_.y + 1 };
			if (isMovable(pos, cells_))
			{
				pos_ = pos;
			}
			else
			{
				field_->fixPiece(pos_, cells_);
				return false;
			}
			break;
		}
		}
		return true;
	}

	bool turn()
	{
		Cell cells[PieceWidth][PieceHeight];

		// 右回転させた座標を入れる
		for (int y = 0; y < PieceHeight; y++)
		{
			for (int x = 0; x < PieceWidth; x++)
			{
				cells[PieceHeight - y - 1][x] = cells_[x][y];
			}
		}

		if (isMovable(pos_, cells))
		{
			for (int y = 0; y < PieceHeight; y++)
			{
				for (int x = 0; x < PieceWidth; x++)
				{
					cells_[x][y] = cells[x][y];
				}
			}
		}
		else
		{
			return false;
		}
		return true;
	}

	Cell getCell(int x, int y) const
	{
		return cells_[x][y];
	}

	Point getPos() const
	{
		return pos_;
	}

private:
	Point pos_;        // 左上の座標
	Field *field_;
	Point pixel_;     // 左上のpixel

	void initPos()
	{
		pos_.x = 5;
		pos_.y = -3;
		pixel_.x = 5 * SizeCellPixelX;
		pixel_.y = -3 * SizeCellPixelY;
	}

	bool isMovable(const Point pos, const Cell cells[][PieceHeight]) const
	{
		for (int y = 0; y < PieceHeight; y++)
		{
			for (int x = 0; x < PieceWidth; x++)
			{
				// 4×4内でブロックのあるマスのみ調べる
				if (cells[x][y].getBlock())
				{
					int ofsX = pos.x + x;
					int ofsY = pos.y + y;
					bool validPos = (0 <= ofsX && ofsX <= FieldWidth - 1)
						&& (0 <= ofsY && ofsY <= FieldHeight - 1);
					if (ofsX < 0 || ofsX >= FieldWidth
						|| ofsY >= FieldHeight
						|| (validPos && field_->getCell(ofsX, ofsY).getBlock()))
					{
						return false;
					}
				}
			}
		}
		return true;
	}
protected:
	Cell cells_[PieceWidth][PieceHeight];
};

// □□□□
// □■■□
// □■■□
// □□□□
class Piece0 : public Piece
{
public:
	Piece0(Field *field)
		: Piece(field)
	{
		cells_[1][1].create(RGB(0x00, 0x8f, 0xff));
		cells_[2][1].create(RGB(0x00, 0x8f, 0xff));
		cells_[1][2].create(RGB(0x00, 0x8f, 0xff));
		cells_[2][2].create(RGB(0x00, 0x8f, 0xff));
	}
};

// □■□□
// □■□□
// □■□□
// □■□□
class Piece1 : public Piece
{
public:
	Piece1(Field *field)
		: Piece(field)
	{
		cells_[1][0].create(RGB(0x4f, 0xaf, 0xdf));
		cells_[1][1].create(RGB(0x4f, 0xaf, 0xdf));
		cells_[1][2].create(RGB(0x4f, 0xaf, 0xdf));
		cells_[1][3].create(RGB(0x4f, 0xaf, 0xdf));
	}
};

// □□□□
// □■□□
// □■■□
// □■□□
class Piece2 : public Piece
{
public:
	Piece2(Field *field)
		: Piece(field)
	{
		cells_[1][1].create(RGB(0x00, 0xff, 0x8f));
		cells_[1][2].create(RGB(0x00, 0xff, 0x8f));
		cells_[2][2].create(RGB(0x00, 0xff, 0x8f));
		cells_[1][3].create(RGB(0x00, 0xff, 0x8f));
	}
};

// □□□□
// □■■□
// □■□□
// □■□□
class Piece3 : public Piece
{
public:
	Piece3(Field *field)
		: Piece(field)
	{
		cells_[1][1].create(RGB(0x00, 0xdf, 0xaf));
		cells_[2][1].create(RGB(0x00, 0xdf, 0xaf));
		cells_[1][2].create(RGB(0x00, 0xdf, 0xaf));
		cells_[1][3].create(RGB(0x00, 0xdf, 0xaf));
	}
};

// □□□□
// □■■□
// □□■□
// □□■□
class Piece4 : public Piece
{
public:
	Piece4(Field *field)
		: Piece(field)
	{
		cells_[1][1].create(RGB(0xff, 0x00, 0x8f));
		cells_[2][1].create(RGB(0xff, 0x00, 0x8f));
		cells_[2][2].create(RGB(0xff, 0x00, 0x8f));
		cells_[2][3].create(RGB(0xff, 0x00, 0x8f));
	}
};

// □□□□
// □□■□
// □■■□
// □■□□
class Piece5 : public Piece
{
public:
	Piece5(Field *field)
		: Piece(field)
	{
		cells_[2][1].create(RGB(0xdf, 0x00, 0xaf));
		cells_[1][2].create(RGB(0xdf, 0x00, 0xaf));
		cells_[2][2].create(RGB(0xdf, 0x00, 0xaf));
		cells_[1][3].create(RGB(0xdf, 0x00, 0xaf));
	}
};

// □□□□
// □■□□
// □■■□
// □□■□
class Piece6 : public Piece
{
public:
	Piece6(Field *field)
		: Piece(field)
	{
		cells_[1][1].create(RGB(0xff, 0x4f, 0x4f));
		cells_[1][2].create(RGB(0xff, 0x4f, 0x4f));
		cells_[2][2].create(RGB(0xff, 0x4f, 0x4f));
		cells_[2][3].create(RGB(0xff, 0x4f, 0x4f));
	}
};

class Tetris
{
public:
	explicit Tetris(HWND hWnd)
		: field_(nullptr), curPiece_(nullptr), nextPiece_(nullptr), hWnd_(hWnd),
		playTime_(0), score_(0), isGameover_(false)
	{
		field_ = new Field();
		nextPiece_ = createPiece();
		curPiece_ = createPiece();
	}

	~Tetris()
	{
		releasePiece(nextPiece_);
		releasePiece(curPiece_);
		delete field_;
	}

	void  releasePiece(Piece *piece)
	{
		if (piece != nullptr)
		{
			delete piece;
			piece = nullptr;
		}
	}

	Piece *createPiece()
	{
		switch (rand() % 7)
		{
		case 0:
			return new Piece0(field_);
		case 1:
			return new Piece1(field_);
		case 2:
			return new Piece2(field_);
		case 3:
			return new Piece3(field_);
		case 4:
			return new Piece4(field_);
		case 5:
			return new Piece5(field_);
		case 6:
			return new Piece6(field_);
		}
		return nullptr;
	}

	Piece *getCurPiece() const
	{
		return curPiece_;
	}

	static UINT WINAPI threadProc(void* parameter)
	{
		return (reinterpret_cast<Tetris*>(parameter))->executeThread();
	}

	void paint(HDC hdc)
	{
		SelectObject(hdc, GetStockObject(NULL_PEN));
		// ゲームフィールドのブロック
		for (int y = 0; y < FieldHeight; y++)
		{
			for (int x = 0; x < FieldWidth; x++)
			{
				int pixelX = (x + 1) * SizeCellPixelX;
				int pixelY = (y + 1) * SizeCellPixelY;
				Cell cell = field_->getCell(x, y);
				drawRectangle(hdc, cell, pixelX, pixelY);
			}
		}

		// 現在移動中のブロック
		for (int y = 0; y < PieceHeight; y++)
		{
			Point pos = curPiece_->getPos();
			if (pos.y + y < 0)
			{
				continue;
			}
			for (int x = 0; x < PieceWidth; x++)
			{
				int pixelX = (pos.x + x + 1) * SizeCellPixelX;
				int pixelY = (pos.y + y + 1) * SizeCellPixelY;
				Cell cell = curPiece_->getCell(x, y);
				drawRectangle(hdc, cell, pixelX, pixelY);
			}
		}

		// 次のブロック
		for (int y = 0; y < PieceHeight; y++)
		{
			for (int x = 0; x < PieceWidth; x++)
			{
				int pixelX = (FieldWidth + 2 + x) * SizeCellPixelX;
				int pixelY = (y + 1) * SizeCellPixelY;
				Cell cell = nextPiece_->getCell(x, y);
				drawRectangle(hdc, cell, pixelX, pixelY);
			}
		}

		SelectObject(hdc, GetStockObject(WHITE_PEN));
		SelectObject(hdc, GetStockObject(NULL_BRUSH));
		Rectangle(hdc,
			(FieldWidth + 2) * SizeCellPixelX, SizeCellPixelY,  // 次のブロックの枠
			(FieldWidth + 2 + PieceWidth) * SizeCellPixelX, (PieceHeight + 1) * SizeCellPixelY);
		Rectangle(hdc,
			SizeCellPixelX, SizeCellPixelY,  // ゲームフィールドの枠
			(FieldWidth + 1) * SizeCellPixelX, (FieldHeight + 1) * SizeCellPixelY);

		drawLine(hdc);

		drawString(hdc);
	}

	bool isGameover()
	{
		return isGameover_;
	}

	void resetState()
	{
		score_ = 0;
		playTime_ = 0;
		isGameover_ = false;
	}


private:
	void drawRectangle(HDC hdc, Cell cell, int pixelX, int pixelY) {
		if (cell.getBlock())
		{
			HBRUSH hBrush = CreateSolidBrush(cell.getColor());
			HBRUSH hOldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, hBrush));
			Rectangle(hdc,
				pixelX,
				pixelY,
				pixelX + SizeCellPixelX,
				pixelY + SizeCellPixelY);
			SelectObject(hdc, hOldBrush);
			DeleteObject(hBrush);
		}
	}
	void drawLine(HDC hdc)
	{
		// DCペンの色を変更する
		SetDCPenColor(hdc, RGB(127, 127, 127));
		SelectObject(hdc, GetStockObject(DC_PEN));
		// 内側描画 インデックスに注意
		for (int x = 1; x < FieldWidth; x++)
		{
			int pixelX = (x + 1) * SizeCellPixelX;
			int pixelY = 1 * SizeCellPixelY;
			MoveToEx(hdc, pixelX, pixelY, NULL);
			LineTo(hdc, pixelX, (FieldHeight + 1) * SizeCellPixelY);
		}
		for (int y = 1; y < FieldHeight; y++)
		{
			int pixelX = 1 * SizeCellPixelX;
			int pixelY = (y + 1) * SizeCellPixelX;
			MoveToEx(hdc, pixelX, pixelY, NULL);
			LineTo(hdc, (FieldWidth + 1) * SizeCellPixelX, pixelY);
		}
	}

	void drawString(HDC hdc)
	{
		TCHAR    buf[16];
		HFONT    hFont;

		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkMode(hdc, TRANSPARENT);    // 背景を残す

		// フォントの設定
		hFont = CreateFont(
			18,            // フォントサイズ
			0, 0, 0,
			FW_NORMAL, FALSE, FALSE, FALSE,
			SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH, _T("Meiryo UI")
		);
		SelectObject(hdc, hFont);

		// 獲得点数
		_stprintf(buf, _T("SCORE"));
		TextOut(hdc, (FieldWidth + 2) * SizeCellPixelX,
			(PieceHeight + 2) * SizeCellPixelY, buf, (int)_tcslen(buf));
		_stprintf(buf, _T("%d"), score_);
		TextOut(hdc, (FieldWidth + 2) * SizeCellPixelX,
			(PieceHeight + 3) * SizeCellPixelY, buf, (int)_tcslen(buf));
		// プレイ時間
		_stprintf(buf, _T("PLAY TIME"));
		TextOut(hdc, (FieldWidth + 2) * SizeCellPixelX,
			(PieceHeight + 5) * SizeCellPixelY, buf, (int)_tcslen(buf));
		_stprintf(buf, _T("%02d:%02d"),
			(playTime_ / 1000) / 60, (playTime_ / 1000) % 60);
		TextOut(hdc, (FieldWidth + 2) * SizeCellPixelX,
			(PieceHeight + 6) * SizeCellPixelY, buf, (int)_tcslen(buf));

		DeleteObject(hFont);
	}

	DWORD WINAPI executeThread()
	{
		// ミューテックスオブジェクトのハンドルを取得
		HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MUTEX_NAME);

		DWORD  signal = -1;
		DWORD  prevTime = timeGetTime();
		DWORD  sleeptime = 1000;
		DWORD  minute = 0;

		while (true)
		{
			// メインスレッド(=勝手に下に動く制御以外)からの介入がない限り
			// タイムアウトによって待機を解除する
			DWORD difftime = timeGetTime() - prevTime;
			// スリープ時間より経過していないならば
			if (difftime < sleeptime)
			{
				// 指定時間経過するとオブジェクトをシグナル状態(=終了)にする。
				// 所有権を持つまで待つ。
				signal = WaitForSingleObject(hMutex, sleeptime - difftime);
			}
			difftime = timeGetTime() - prevTime;
			playTime_ += difftime;
			minute += difftime;
			// 1分ごとに待機時間を減らしてゆく
			if (minute >= 60 * 1000 && sleeptime > 100)
			{
				sleeptime -= 100;
				minute = 0;
			}
			prevTime = timeGetTime();

			bool isSucceeded = curPiece_->move(Piece::Down);
			if (!isSucceeded)
			{
				int line = field_->deleteLine();
				if (line > 0)
				{
					// 移動中のブロック自体を0にしてやる
					curPiece_->clear();
					if (line == 4)
					{
						score_ += 1000;
					}
					else
					{
						score_ += (line * 100);
					}
					InvalidateRect(hWnd_, NULL, FALSE);
					// 消した瞬間を演出
					Sleep(500);
					field_->shiftLine(line);
				}
				if (field_->isStacked())
				{
					isGameover_ = true;
					ReleaseMutex(hMutex);   // ミューテックスの所有権を解放
					CloseHandle(hMutex);    // 全てのハンドルを閉じない限りミューテックスは破棄されない
					int ans = MessageBox(hWnd_, _T("Continue ?"), _T("Gameover"), MB_YESNO);
					if (ans == IDYES)
					{
						SendMessage(hWnd_, WM_RESTART, 0, 0);
					}
					else if (ans == IDNO)
					{
						SendMessage(hWnd_, WM_DESTROY, 0, 0);
					}
					return 0;       // ゲーム終了でスレッドは破棄する
				}
				// 新しいピースを生成
				releasePiece(curPiece_);
				curPiece_ = nextPiece_;
				nextPiece_ = createPiece();
			}
			InvalidateRect(hWnd_, NULL, FALSE);
			// オブジェクトがシグナル状態(=終了)なら
			if (signal == WAIT_OBJECT_0)
			{
				// ミュー手クスオブジェクトの所有権を解放する
				ReleaseMutex(hMutex);
				SendMessage(hWnd_, WM_MUTEX, 0, 0);
			}

		}
		return 0;
	}

	HWND      hWnd_;
	Piece     *nextPiece_;
	Piece     *curPiece_;
	Field     *field_;
	int       playTime_;
	int       score_;
	bool      isGameover_;

};


INT_PTR CALLBACK about(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return static_cast<INT_PTR>(TRUE);

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return static_cast<INT_PTR>(TRUE);
		}
		break;
	}
	return static_cast<INT_PTR>(FALSE);
}


class TetrisWindow {
public:
	TetrisWindow() { }
	virtual ~TetrisWindow() { }

	void create(HINSTANCE hInstance, int nCmdShow)
	{
		LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
		LoadString(hInstance, IDC_WIN32TETRIS, szWindowClass, MAX_LOADSTRING);
		myRegisterClass(hInstance);
		// アプリケーションの初期化を実行します:
		if (!initInstance(hInstance, nCmdShow))
		{
			return;
		}

		MSG msg;
		HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32TETRIS));

		// メイン メッセージ ループ
		while (GetMessage(&msg, NULL, 0, 0))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

private:

	Tetris  *tetris;
	HWND    hWnd;
	HANDLE  hMutex;
	HANDLE  hThread;
	HDC     hMemDC;
	HDC     hTmpDC;
	HBITMAP hBackBitmap;
	HBITMAP hFrontBitmap;
	HINSTANCE hInst;
	TCHAR szTitle[MAX_LOADSTRING];          // タイトル バーのテキスト
	TCHAR szWindowClass[MAX_LOADSTRING];    // メイン ウィンドウ クラス名

	ATOM myRegisterClass(HINSTANCE hInstance)
	{
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = staticBaseWndProc; // staticメンバ関数を渡す
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32TETRIS));
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
		wcex.lpszMenuName = MAKEINTRESOURCE(IDC_WIN32TETRIS);
		wcex.lpszClassName = szWindowClass;
		wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		return RegisterClassEx(&wcex);
	}

	BOOL initInstance(HINSTANCE hInstance, int nCmdShow)
	{
		HWND hWnd;
		hInst = hInstance;
		RECT rc = {
			0, 0,
			(FieldWidth + 7) * SizeCellPixelX, (FieldHeight + 2) * SizeCellPixelY
		};
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME, TRUE);

		hWnd = CreateWindow(szWindowClass, szTitle,
			(WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX) | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			rc.right - rc.left,   // Windowの幅
			rc.bottom - rc.top,   // Windowの高さ
			NULL, NULL, hInstance,
			this); // 自クラスのポインタを自由な領域に格納

		if (!hWnd)
		{
			return FALSE;
		}

		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);

		return TRUE;
	}

	HANDLE reInitialize(HWND hWnd)
	{
		delete tetris;
		tetris = new Tetris(hWnd);
		UINT uId;
		return reinterpret_cast<HANDLE>(
			_beginthreadex(
				NULL,                // security
				0,                   // stack_size for a new thread, or 0
				Tetris::threadProc,  // start_address
				reinterpret_cast<void *>(tetris),  // arglist
				0,                   // initflag 0 for running
				&uId)                // thrdaddr (identifier)
			);
	}

	void setPointer(HWND hWnd)
	{
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		this->hWnd = hWnd;
	}

	void onCreate(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		HDC     hdc = nullptr;
		UINT    uId;

		tetris = new Tetris(hWnd);
		hdc = GetDC(hWnd);      // ウィンドウのHDCを取得
								// 背景画面 黒色画像を作る(初期状態のDDBは黒で塗りつぶされている)
								// ウィンドウのHDCと互換性のあるDDB作成
		hBackBitmap = CreateCompatibleBitmap(hdc, SizeBmPixelX, SizeBmPixelY);

		hMemDC = CreateCompatibleDC(hdc);   // ウィンドウのHDCと互換性のあるDCを作成
		SelectObject(hMemDC, hBackBitmap);  // DDBをデバイスコンテキストに設定

											// 更新画面 黒色画像を作る(初期状態のDDBは黒で塗りつぶされている)
											// ウィンドウのHDCと互換性のあるDDB作成
		hFrontBitmap = CreateCompatibleBitmap(hdc, SizeBmPixelX, SizeBmPixelY);
		hTmpDC = CreateCompatibleDC(hdc);    // ウィンドウのHDCと互換性のあるDCを作成 
		SelectObject(hTmpDC, hFrontBitmap);  // DDBをデバイスコンテキストに設定
		ReleaseDC(hWnd, hdc);   // ウィンドウのHDCを解放

		srand(static_cast<UINT>(time(NULL)));
		hMutex = CreateMutex(NULL, TRUE, MUTEX_NAME);
		hThread = reinterpret_cast<HANDLE>(
			_beginthreadex(
				NULL,                // security
				0,                   // stack_size for a new thread, or 0
				Tetris::threadProc,  // start_address
				reinterpret_cast<void *>(tetris),  // arglist
				0,                   // initflag 0 for running
				&uId)                // thrdaddr (identifier)
			);
	}

	void onCommand(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		int wmId, wmEvent;
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, about);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			DefWindowProc(hWnd, message, wParam, lParam);
			return;
		}
	}

	void onPaint(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// 背景部を描画 ダブルバッファリング
		BitBlt(hTmpDC, 0, 0, SizeBmPixelX, SizeBmPixelY,
			hMemDC, 0, 0, SRCCOPY);
		tetris->paint(hTmpDC);
		// 前面部を描画 ダブルバッファリング
		BitBlt(hdc, 0, 0, SizeBmPixelX, SizeBmPixelY,
			hTmpDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
	}

	void onDestroy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		DeleteObject(hBackBitmap);
		DeleteObject(hFrontBitmap);
		DeleteDC(hMemDC);
		DeleteDC(hTmpDC);
		CloseHandle(hThread);
		CloseHandle(hMutex);
		delete tetris;
		PostQuitMessage(0);
	}

	void onKeyDown(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		bool isSucceeded = false;
		Point pixel = { 0 }; // ADD
		switch (wParam)
		{
		case VK_LEFT:
			isSucceeded = tetris->getCurPiece()->move(Piece::Left);
			break;
		case VK_RIGHT:
			isSucceeded = tetris->getCurPiece()->move(Piece::Right);
			break;
		case VK_DOWN:
			isSucceeded = tetris->getCurPiece()->move(Piece::Down);
			if (!isSucceeded)
			{
				ReleaseMutex(hMutex);
			}
			break;
		case VK_SPACE:
			isSucceeded = tetris->getCurPiece()->turn();
			break;
		}
		if (isSucceeded)
		{
			InvalidateRect(hWnd, NULL, FALSE);
		}
	}

	void onMutex(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		// オブジェクトがシグナル状態(=終了)になるまで待機
		WaitForSingleObject(hMutex, INFINITE);
	}

	void onRestart(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (tetris->isGameover())
		{
			WaitForSingleObject(hMutex, INFINITE);
			CloseHandle(hThread);
			hThread = reInitialize(hWnd);
		}
	}

	static LRESULT CALLBACK staticBaseWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{

		// インスタンスのポインターを取得する
		TetrisWindow *tetrisWindow = reinterpret_cast<TetrisWindow *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (tetrisWindow == nullptr) {
			if (message == WM_CREATE) {
				// CreateWindowのパラメータから取得する
				tetrisWindow = reinterpret_cast<TetrisWindow *>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
				// windowオブジェクトとウィンドウハンドルを関連付ける
				tetrisWindow->setPointer(hWnd);
			}
		}

		switch (message)
		{
		case WM_CREATE:
			tetrisWindow->onCreate(hWnd, message, wParam, lParam);
			break;
		case WM_COMMAND:
			tetrisWindow->onCommand(hWnd, message, wParam, lParam);
			break;
		case WM_PAINT:
			tetrisWindow->onPaint(hWnd, message, wParam, lParam);
			break;
		case WM_DESTROY:
			tetrisWindow->onDestroy(hWnd, message, wParam, lParam);
			break;
		case WM_KEYDOWN:
			tetrisWindow->onKeyDown(hWnd, message, wParam, lParam);
			break;
		case WM_MUTEX:
			tetrisWindow->onMutex(hWnd, message, wParam, lParam);
			break;
		case WM_RESTART:
			tetrisWindow->onRestart(hWnd, message, wParam, lParam);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

};

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	TetrisWindow tetrisWindow;
	tetrisWindow.create(hInstance, nCmdShow);

	return 0;
}
