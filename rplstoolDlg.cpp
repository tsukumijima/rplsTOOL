
// rplstoolDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include <filesystem>

#include "rplstool.h"
#include "rplstoolDlg.h"
#include "convToUnicode.h"
#include "proginfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define		CONVBUFSIZE			65536
#define		RPLSFILESIZE		1048576
#define		DEFAULTTSFILEPOS	50
#define		DEFAULTPACKETLIMIT	0

#define		NAMESTRING			L"rplsTOOL"
#define		NAMESTRING2			L"rplsTOOL *"

#ifdef		_WIN64
	#define		NAMESTRING3		L", Version 2.10 (64bit) "
#else
	#define		NAMESTRING3		L", Version 2.10 (32bit) "
#endif

#define		TOOLSECTION			L"TOOLOPTIONS"
#define		BCHARSIZETO			L"APPLYCHARSIZE"
#define		BCHARSIZEFROM		L"SAVECHARSIZE"
#define		BUSEIVS				L"USEIVS"
#define		BQUICKEXIT			L"QUICKEXIT"
#define		BFORCENORMALRPLS	L"FORCENORMALRPLS"
#define		BSHOWMAX			L"SHOWMAX"
#define		NTSFILEPOS			L"TSFILEPOS"
#define		NPACKETLIMIT		L"PACKETLIMIT"
#define		STRFONTNAME			L"FONTNAME"
#define		NFONTSIZE			L"FONTSIZE"

#define		COPYSECTION			L"COPYOPTIONS"
#define		BFNAME				L"COPYFILENAME"
#define		BRECDATE			L"COPYRECDATE"
#define		BRECTIME			L"COPYRECTIME"
#define		BDURTIME			L"COPYRECDURATION"
#define		BTIMEZONE			L"COPYTIMEZONE"
#define		BMAKERID			L"COPYMAKERID"
#define		BMODELCODE			L"COPYMODELCODE"
#define		BRECSRC				L"COPYRECSRC"
#define		BCHNAME				L"COPYCHANNELNAME"
#define		BCHNUM				L"COPYCHANNELNUM"
#define		BPNAME				L"COPYPROGRAMNAME"
#define		BPDETAIL			L"COPYPROGRAMDETAIL"
#define		PEXTEND				L"COPYPROGRAMEXTEND"
#define		PGENRE				L"COPYPROGRAMGENRE"

#define		WINDOWSECTION		L"WINDOW"
#define		WINDOWPOS			L"WINDOWPOSITION"

#define		MDIVX(x)			MulDiv(x,baseX,4)				// ダイアログ単位からピクセル数への変換用
#define		MDIVY(x)			MulDiv(x,baseY,8)
#define		_BTOINT(x)			(x ? 1 : 0)

#define		STR_NEWLINE			L"\r\n"

#define		OPT_WINDOW_POS		L"--set-window-pos"


// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CrplstoolDlg ダイアログ


CrplstoolDlg::CrplstoolDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CrplstoolDlg::IDD, pParent)
	, filebuf(NULL)
	, savebuf(NULL)
	, baseX(0)
	, baseY(0)
	, initialH(0)
	, initialW(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CrplstoolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CrplstoolDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_FSELECT, &CrplstoolDlg::OnBnClickedFselect)
	ON_CBN_SELENDOK(IDC_GENREL, &CrplstoolDlg::OnCbnSelendokGenrel)
	ON_BN_CLICKED(IDC_RELOAD, &CrplstoolDlg::OnBnClickedReload)
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDCANCEL, &CrplstoolDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &CrplstoolDlg::OnBnClickedOk)
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_WM_RBUTTONUP()
	ON_COMMAND_RANGE(ID__MENU_TEXT, ID__MENU_CSV, &CrplstoolDlg::OnMenuTextCopy)
	ON_COMMAND_RANGE(ID__MENU_FNAME, ID__MENU_RECSRC, &CrplstoolDlg::OnMenuPItemSelect)
	ON_CBN_SELCHANGE(IDC_GENREL2, &CrplstoolDlg::OnCbnSelchangeGenrel2)
	ON_CBN_SELCHANGE(IDC_GENREL3, &CrplstoolDlg::OnCbnSelchangeGenrel3)
END_MESSAGE_MAP()


// CrplstoolDlg メッセージ ハンドラ

BOOL CrplstoolDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。

	DragAcceptFiles(TRUE);

	RECT	rect = {4, 8, 0, 0};
	MapDialogRect(&rect);				// ダイアログベースユニット取得(ウインドウリサイズ時の計算用)
	baseX = rect.left;
	baseY = rect.top;

	GetWindowRect(&rect);				// ウインドウ初期サイズ取得
	initialW = rect.right;
	initialH = rect.bottom;

	try {
		filebuf = new uint8_t[RPLSFILESIZE];											// rplsファイル読み込み、保存用バッファメモリ
		savebuf = new uint8_t[RPLSFILESIZE];
	}
	catch(...) {
		MessageBox(L"アプリケーションバッファメモリの確保に失敗しました", NAMESTRING, MB_ICONSTOP | MB_OK);
		CDialog::OnCancel();															// メモリ確保に失敗したら終了する
		return TRUE;
	}

	SendDlgItemMessage(IDC_CHANNELNAME, EM_LIMITTEXT, (WPARAM)16384,  0L);				// エディットボックスのテキストサイズ限度指定
	SendDlgItemMessage(IDC_CHANNELNUM , EM_LIMITTEXT, (WPARAM)16384,  0L); 
	SendDlgItemMessage(IDC_PNAME      , EM_LIMITTEXT, (WPARAM)16384,  0L); 
	SendDlgItemMessage(IDC_PDETAIL    , EM_LIMITTEXT, (WPARAM)16384,  0L); 

	hRplsFile	= INVALID_HANDLE_VALUE;

	CrplstoolDlg::LoadToolOptions();													// .iniファイルからのオプション設定の読み込み
	CrplstoolDlg::SetEditBoxFont();														// 各エディットボックス，コンボボックスに対するフォントの指定			
	CrplstoolDlg::ParamInit();															// データ初期化

	int32_t		argn;
	LPWSTR		*args = CommandLineToArgvW(GetCommandLine(), &argn);					// コマンドライン文字列取得
	rplstoolpath = args[0];																// アプリケーション本体のパス

	int32_t		offset = 0;
	
	if ((argn == 4) && (wcscmp(args[2], OPT_WINDOW_POS) == 0)) {
		offset = _wtoi(args[3]);														// ウインドウ位置指定オプションが存在する場合、オフセットを取得する
		argn = 2;
	}

	if(argn > 1) {
		CrplstoolDlg::FileReadAndCheck(args[1]);										// ファイル名指定があれば読み込み、データ取得
	}

	CrplstoolDlg::ParamUpdate(false);													// アプリケーションウインドウにデータ反映

	WINDOWPLACEMENT		*wp;
	UINT				n;

	if(AfxGetApp()->GetProfileBinary(WINDOWSECTION, WINDOWPOS, (LPBYTE*)&wp, &n)) {		// .iniファイルからウィンドウ位置を取得する
		int32_t  s = GetSystemMetrics(SM_CYCAPTION);									// タイトルバーの高さを取得する

		wp->rcNormalPosition.left   += (offset * s);
		wp->rcNormalPosition.top    += (offset * s);
		wp->rcNormalPosition.right  += (offset * s);
		wp->rcNormalPosition.bottom += (offset * s);
		SetWindowPlacement(wp);
		
		delete [] wp;
	}

	if(opt.bShowMax) ShowWindow(SW_SHOWMAXIMIZED);								// ウインドウ最大化

	for (int32_t i = 2; i < argn; i++) {
		CrplstoolDlg::OpenNewAppProcess(args[i], i - 1);						// 複数の対象ファイルが指定されている場合、2つ目以降は別個にアプリケーションを起動する
		Sleep(100);
	}

	LocalFree(args);															// CommandLineToArgvW()で確保されたメモリの開放

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CrplstoolDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CrplstoolDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CrplstoolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CrplstoolDlg::OnBnClickedFselect()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	// ファイル選択ボタンが押された場合の処理

	if( (hRplsFile != INVALID_HANDLE_VALUE) && CrplstoolDlg::ParamModCheck() ) {
		int32_t	result = MessageBox(L"開いているファイルの変更内容は失われます\n宜しいですか？", NAMESTRING, MB_ICONEXCLAMATION | MB_OKCANCEL);
		if(result != IDOK) return;														// 確認ダイアログでキャンセルされたら中止して戻る
	}

	CString		ffilter(L"rpls files (*.rpls)|*.rpls|all files (*.*)|*.*||");
	CFileDialog	fSelDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, ffilter);

	if( fSelDlg.DoModal() != IDOK ) return;												// ファイル選択ダイアログでキャンセルされたら戻る
	CString		fname = fSelDlg.GetPathName();											// ファイル名取得

	if(hRplsFile != INVALID_HANDLE_VALUE) CloseHandle(hRplsFile);						// 開いているファイルがあったら閉じる

	if( !CrplstoolDlg::FileReadAndCheck((LPCWSTR)fname) ) CrplstoolDlg::ParamInit();	// ファイルを読み込んでデータ取得、失敗したら初期化
	CrplstoolDlg::ParamUpdate(false);

	return;
}


void CrplstoolDlg::SetComboGenre(const int32_t* genre)
{
	CrplstoolDlg::SetComboGenreL(IDC_GENREL, IDC_GENREM, genre[0]);

	GetDlgItem(IDC_GENREL)->EnableWindow(src.bSonyRpls || src.bPanaRpls);																				// !bSonyRplsかつ!bPanaRplsで無効表示
	GetDlgItem(IDC_GENREM)->EnableWindow(src.bSonyRpls || src.bPanaRpls);

	CrplstoolDlg::SetComboGenreL(IDC_GENREL2, IDC_GENREM2, genre[1]);

	GetDlgItem(IDC_GENREL2)->EnableWindow( (src.bSonyRpls || src.bPanaRpls) && (genre[0] != -1) );
	GetDlgItem(IDC_GENREM2)->EnableWindow( (src.bSonyRpls || src.bPanaRpls) && (genre[0] != -1) );

	CrplstoolDlg::SetComboGenreL(IDC_GENREL3, IDC_GENREM3, genre[2]); 

	GetDlgItem(IDC_GENREL3)->EnableWindow( (src.bSonyRpls || src.bPanaRpls) && (genre[0] != -1) && (genre[1] != -1) );
	GetDlgItem(IDC_GENREM3)->EnableWindow( (src.bSonyRpls || src.bPanaRpls) && (genre[0] != -1) && (genre[1] != -1) );

	return;
}


void CrplstoolDlg::SetComboGenreL(const int32_t idGenreL, const int32_t idGenreM, const int32_t genre)
{
	// 番組ジャンル情報（大分類，中分類）のコンボボックス表示設定

	CComboBox	*genreL = (CComboBox*)GetDlgItem(idGenreL);

	if(genreL->GetCount() == 0)
	{
		genreL->AddString(L"");

		for(int32_t i = 0; i < 16; i++) {
			CString str;
			GetGenreStrL(i, str);
			genreL->AddString(str);
		}
	}

	if(genre != -1) {
		genreL->SetCurSel((genre >> 4) + 1);
	} else {
		genreL->SetCurSel(-1);
	}

	CrplstoolDlg::SetComboGenreM(idGenreM, genre);	// 中分類の設定

	return;
}


void CrplstoolDlg::GetGenreStrL(const int32_t genre, CString& str)
{
	WCHAR	*str_genreL[] = {
		L"ニュース／報道",			L"スポーツ",	L"情報／ワイドショー",	L"ドラマ",
		L"音楽",					L"バラエティ",	L"映画",				L"アニメ／特撮",
		L"ドキュメンタリー／教養",	L"劇場／公演",	L"趣味／教育",			L"福祉",
		L"予備",					L"予備",		L"拡張",				L"その他"
	};

	str.Format(L"%s", str_genreL[genre]);

	return;
}


void CrplstoolDlg::SetComboGenreM(int32_t idGenreM, int32_t genre)
{
	// 番組ジャンル情報（中分類）のコンボボックス設定

	CComboBox	*genreM = (CComboBox*)GetDlgItem(idGenreM);

	genreM->ResetContent();

	if(genre != -1)
	{
		for(int32_t i = 0; i < 16; i++) {
			CString	str;
			GetGenreStrM((genre & 0xf0) + i, str);
			genreM->AddString(str);
		}
		genreM->SetCurSel(genre & 0x0f);
	}
	else
	{
		genreM->SetCurSel(-1);
	}

	return;
}


void CrplstoolDlg::GetGenreStrM(const int32_t genre, CString& str)
{
	WCHAR	*str_genreM[] = {
		L"定時・総合", L"天気", L"特集・ドキュメント", L"政治・国会", L"経済・市況", L"海外・国際", L"解説", L"討論・会談",
		L"報道特番", L"ローカル・地域", L"交通", L"-", L"-", L"-", L"-", L"その他",

		L"スポーツニュース", L"野球", L"サッカー", L"ゴルフ", L"その他の球技", L"相撲・格闘技", L"オリンピック・国際大会", L"マラソン・陸上・水泳",
		L"モータースポーツ", L"マリン・ウィンタースポーツ", L"競馬・公営競技", L"-", L"-", L"-", L"-", L"その他",

		L"芸能・ワイドショー", L"ファッション", L"暮らし・住まい", L"健康・医療", L"ショッピング・通販", L"グルメ・料理", L"イベント", L"番組紹介・お知らせ",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"国内ドラマ", L"海外ドラマ", L"時代劇", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"国内ロック・ポップス", L"海外ロック・ポップス", L"クラシック・オペラ", L"ジャズ・フュージョン", L"歌謡曲・演歌", L"ライブ・コンサート", L"ランキング・リクエスト", L"カラオケ・のど自慢",
		L"民謡・邦楽", L"童謡・キッズ", L"民族音楽・ワールドミュージック", L"-", L"-", L"-", L"-", L"その他",

		L"クイズ", L"ゲーム", L"トークバラエティ", L"お笑い・コメディ", L"音楽バラエティ", L"旅バラエティ", L"料理バラエティ", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"洋画", L"邦画", L"アニメ", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"国内アニメ", L"海外アニメ", L"特撮", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"社会・時事", L"歴史・紀行", L"自然・動物・環境", L"宇宙・科学・医学", L"カルチャー・伝統芸能", L"文学・文芸", L"スポーツ", L"ドキュメンタリー全般",
		L"インタビュー・討論", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"現代劇・新劇", L"ミュージカル", L"ダンス・バレエ", L"落語・演芸", L"歌舞伎・古典", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"旅・釣り・アウトドア", L"園芸・ペット・手芸", L"音楽・美術・工芸", L"囲碁・将棋", L"麻雀・パチンコ", L"車・オートバイ", L"コンピュータ・ＴＶゲーム", L"会話・語学",
		L"幼児・小学生", L"中学生・高校生", L"大学生・受験", L"生涯教育・資格", L"教育問題", L"-", L"-", L"その他",

		L"高齢者", L"障害者", L"社会福祉", L"ボランティア", L"手話", L"文字（字幕）", L"音声解説", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"BS/地上デジタル放送用番組付属情報", L"広帯域CSデジタル放送用拡張", L"衛星デジタル音声放送用拡張", L"サーバー型番組付属情報", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"その他"
	};

	str.Format(L"%s", str_genreM[genre]);

	return;
}


int32_t CrplstoolDlg::GetComboGenre(const int32_t idGenreL, const int32_t idGenreM)
{
	CComboBox	*genreL = (CComboBox*)GetDlgItem(idGenreL);
	CComboBox	*genreM = (CComboBox*)GetDlgItem(idGenreM);

	int32_t genre = -1;

	if(genreL->GetCurSel() > 0) {
		genre = (genreL->GetCurSel() - 1) * 16 + genreM->GetCurSel();
	}

	return genre;
}


void CrplstoolDlg::OnCbnSelendokGenrel()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	// 番組ジャンル情報大分類に選択変更があった場合、中分類を再設定する

	CComboBox	*genreL = (CComboBox*)GetDlgItem(IDC_GENREL);

	int32_t		cursel = genreL->GetCurSel();

	if(cursel == CB_ERR) return;

	if(cursel == 0) {
		CrplstoolDlg::SetComboGenreM(IDC_GENREM, -1);
		CrplstoolDlg::SetComboGenreL(IDC_GENREL2, IDC_GENREM2, -1);
		CrplstoolDlg::SetComboGenreL(IDC_GENREL3, IDC_GENREM3, -1);
	} else {
		CrplstoolDlg::SetComboGenreM(IDC_GENREM, (cursel - 1) << 4);
	}

	GetDlgItem(IDC_GENREL2)->EnableWindow(cursel != 0);
	GetDlgItem(IDC_GENREM2)->EnableWindow(cursel != 0);

	GetDlgItem(IDC_GENREL3)->EnableWindow( (cursel != 0) && (((CComboBox*)GetDlgItem(IDC_GENREL2))->GetCurSel() > 0) );
	GetDlgItem(IDC_GENREM3)->EnableWindow( (cursel != 0) && (((CComboBox*)GetDlgItem(IDC_GENREL2))->GetCurSel() > 0) );
	
	return;
}


void CrplstoolDlg::OnCbnSelchangeGenrel2()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	// 番組ジャンル情報大分類に選択変更があった場合、中分類を再設定する

	CComboBox	*genreL = (CComboBox*)GetDlgItem(IDC_GENREL2);

	int32_t		cursel = genreL->GetCurSel();

	if(cursel == CB_ERR) return;

	if(cursel == 0) {
		CrplstoolDlg::SetComboGenreM(IDC_GENREM2, -1);
		CrplstoolDlg::SetComboGenreL(IDC_GENREL3, IDC_GENREM3, -1);
	} else {
		CrplstoolDlg::SetComboGenreM(IDC_GENREM2, (cursel - 1) << 4);
	}

	GetDlgItem(IDC_GENREL3)->EnableWindow(cursel != 0);
	GetDlgItem(IDC_GENREM3)->EnableWindow(cursel != 0);

	return;
}


void CrplstoolDlg::OnCbnSelchangeGenrel3()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	// 番組ジャンル情報大分類に選択変更があった場合、中分類を再設定する

	CComboBox	*genreL = (CComboBox*)GetDlgItem(IDC_GENREL3);

	int32_t		cursel = genreL->GetCurSel();

	if(cursel == CB_ERR) return;

	if(cursel == 0) {
		CrplstoolDlg::SetComboGenreM(IDC_GENREM3, -1);
	} else {
		CrplstoolDlg::SetComboGenreM(IDC_GENREM3, (cursel - 1) << 4);
	}

	return;
}


void CrplstoolDlg::SetComboRecSrc(const int32_t index)
{
	// 放送種別（録画ソース）のコンボボックス表示設定

	CComboBox	*recsrc = (CComboBox*)GetDlgItem(IDC_RECSRC);

	if(recsrc->GetCount() == 0)
	{
		for(int32_t i = 0;; i++) {
			CString str;
			GetComboStrRecSrc(i, str);
			if(str == L"") break;
			recsrc->AddString(str);
		}
	}

	if(index != -1) {
		recsrc->SetCurSel(index);
	} else {
		recsrc->SetCurSel(-1);
	}

	return;
}


void CrplstoolDlg::GetComboStrRecSrc(const int32_t index, CString& str)
{
	WCHAR	*str_recsrc[] = {
		L"地上デジタル",
		L"BSデジタル",
		L"CSデジタル1",
		L"CSデジタル2",
		L"i.LINK(TS)",
		L"AVCHD",
		L"スカパー(DLNA)",
		L"DV入力",
		L"地上アナログ",
		L"ライン入力",
	};

	if( (index >= 0) && (index < (sizeof(str_recsrc) / sizeof(WCHAR*))) ) {
		str.Format(L"%s", str_recsrc[index]);
	} else {
		str = L"";
	}

	return;
}


int32_t CrplstoolDlg::GetRecSrcIndex(const int32_t num, const bool bDir)
{
	// bDir:true		引数numをrecsrc値とみなしてindex値を返す
	// bDir:false		引数numをindex値とみなしてrecsrc値を返す

	int32_t		recsrc_table[] =
	{								// 順序はGetComboStrRecSrcと対応する必要あり
		0x5444,						// TD	地上デジタル
		0x4244,						// BD	BSデジタル
		0x4331,						// C1	CSデジタル1
		0x4332,						// C2	CSデジタル2
		0x694C,						// iL	i.LINK(TS)入力
		0x4D56,						// MV	AVCHD
		0x534B,						// SK	スカパー(DLNA)
		0x4456,						// DV	DV入力
		0x5441,						// TA	地上アナログ
		0x4E4C,						// NL	ライン入力
	};

	if(!bDir){
		if( (num >= 0) && (num < (sizeof(recsrc_table) / sizeof(int))) ) {
			return	recsrc_table[num];
		} else {
			return -1;
		}
	}

	// 小さなテーブルなので順次探索で

	for(int32_t i = 0; i < (sizeof(recsrc_table) / sizeof(int32_t)); i++) {
		if(num == recsrc_table[i]) return i;
	}

	return -1;
}


void CrplstoolDlg::ParamInit(void)
{
	// 全パラメータの初期化

	src.filename	= L"";
	src.shortfname	= L"";
	src.filelen		= 0;

	src.recyear		= 0;
	src.recmonth	= 0;
	src.recday		= 0;
	src.rechour		= 0;
	src.recmin		= 0;
	src.recsec		= 0;
	src.rectimezone	= 0;

	src.durhour		= 0;
	src.durmin		= 0;
	src.dursec		= 0;

	src.makerid		= 0;
	src.modelcode	= 0;

	src.recsrc		= -1;
	src.chname		= L"";
	src.chnum		= L"000";

	src.pname		= L"";
	src.pdetail		= L"";
	src.pdetaillen	= 0;
	src.pextend		= L"";
	src.pextendlen	= 0;

	src.bSonyRpls	= !opt.bForceNormalRpls;
	src.bPanaRpls	= false;
	src.genre[0]	= -1;
	src.genre[1]	= -1;
	src.genre[2]	= -1;

	src.bReadOnly	= false;

	bak = src;

	return;
}

void CrplstoolDlg::ParamUpdate(const bool bDir)
{
	// アプリケーションウインドウへのデータ反映，もしくはアプリケーションウインドウからのデータ取得

	WCHAR	str[32];

	if(!bDir) {																	// データ反映

		CString		windowTitle(opt.bCharSizeFrom ? NAMESTRING : NAMESTRING2);
		if (src.shortfname != L"") windowTitle = src.shortfname + L" - " + windowTitle;
		this->SetWindowText(windowTitle);

		GetDlgItem(IDC_FNAME)->SetWindowTextW(src.filename);

		_itow_s(src.recyear, str, 16, 10);
		GetDlgItem(IDC_RECYEAR)->SetWindowTextW(str);
		_itow_s(src.recmonth, str, 16, 10);
		GetDlgItem(IDC_RECMONTH)->SetWindowTextW(str);
		_itow_s(src.recday, str, 16, 10);
		GetDlgItem(IDC_RECDAY)->SetWindowTextW(str);
		_itow_s(src.rechour, str, 16, 10);
		GetDlgItem(IDC_RECHOUR)->SetWindowTextW(str);
		_itow_s(src.recmin, str, 16, 10);
		GetDlgItem(IDC_RECMIN)->SetWindowTextW(str);
		_itow_s(src.recsec, str, 16, 10);
		GetDlgItem(IDC_RECSEC)->SetWindowTextW(str);
		_itow_s(src.rectimezone, str, 16, 10);
		GetDlgItem(IDC_TIMEZONE)->SetWindowTextW(str);

		_itow_s(src.durhour, str, 16, 10);
		GetDlgItem(IDC_DURHOUR)->SetWindowTextW(str);
		_itow_s(src.durmin, str, 16, 10);
		GetDlgItem(IDC_DURMIN)->SetWindowTextW(str);
		_itow_s(src.dursec, str, 16, 10);
		GetDlgItem(IDC_DURSEC)->SetWindowTextW(str);

		_itow_s(src.makerid, str, 16, 10);
		GetDlgItem(IDC_MAKERID)->SetWindowTextW(str);
		_itow_s(src.modelcode, str, 16, 10);
		GetDlgItem(IDC_MODELCODE)->SetWindowTextW(str);

		CrplstoolDlg::SetComboRecSrc(src.recsrc);

		GetDlgItem(IDC_CHANNELNAME)->SetWindowTextW(src.chname);
		GetDlgItem(IDC_CHANNELNUM)->SetWindowTextW(src.chnum);

		GetDlgItem(IDC_PNAME)->SetWindowTextW(src.pname);
		GetDlgItem(IDC_PDETAIL)->SetWindowTextW(src.pdetail);
		GetDlgItem(IDC_PEXTEND)->SetWindowTextW(src.pextend);

		CrplstoolDlg::SetComboGenre(src.genre);

		GetDlgItem(IDC_TEXTRECSRC)->ShowWindow(src.bPanaRpls ? SW_SHOW : SW_HIDE);																			// bPanaRplsに従って「放送種別」を表示するか否か決める
		GetDlgItem(IDC_RECSRC)->ShowWindow(src.bPanaRpls ? SW_SHOW : SW_HIDE);

		RECT	rect;
		GetClientRect(&rect);
		GetDlgItem(IDC_PDETAIL)->MoveWindow(MDIVX(56), MDIVY(166), rect.right - MDIVX(73), src.bSonyRpls ? MDIVY(36) : rect.bottom - MDIVY(260), TRUE);		// bSonyRplsに従って「番組内容」欄の表示サイズを変更する

		GetDlgItem(IDC_PEXTEND)->ShowWindow(src.bSonyRpls ? SW_SHOW : SW_HIDE);																				// !bSonyRplsで非表示
		GetDlgItem(IDC_TEXTPEXTEND)->ShowWindow(src.bSonyRpls ? SW_SHOW : SW_HIDE);

		GetDlgItem(IDOK)->EnableWindow(hRplsFile != INVALID_HANDLE_VALUE);

	} else {																	// データ取得

		GetDlgItem(IDC_RECYEAR)->GetWindowTextW(str, 16);
		src.recyear = _wtoi(str);
		GetDlgItem(IDC_RECMONTH)->GetWindowTextW(str, 16);
		src.recmonth = _wtoi(str);
		GetDlgItem(IDC_RECDAY)->GetWindowTextW(str, 16);
		src.recday = _wtoi(str);
		GetDlgItem(IDC_RECHOUR)->GetWindowTextW(str, 16);
		src.rechour = _wtoi(str);
		GetDlgItem(IDC_RECMIN)->GetWindowTextW(str, 16);
		src.recmin = _wtoi(str);
		GetDlgItem(IDC_RECSEC)->GetWindowTextW(str, 16);
		src.recsec = _wtoi(str);
		GetDlgItem(IDC_TIMEZONE)->GetWindowTextW(str, 16);
		src.rectimezone = _wtoi(str);

		GetDlgItem(IDC_DURHOUR)->GetWindowTextW(str, 16);
		src.durhour = _wtoi(str);
		GetDlgItem(IDC_DURMIN)->GetWindowTextW(str, 16);
		src.durmin = _wtoi(str);
		GetDlgItem(IDC_DURSEC)->GetWindowTextW(str, 16);
		src.dursec = _wtoi(str);

		GetDlgItem(IDC_MAKERID)->GetWindowTextW(str, 16);
		src.makerid = _wtoi(str);
		GetDlgItem(IDC_MODELCODE)->GetWindowTextW(str, 16);
		src.modelcode = _wtoi(str);

		CComboBox	*recsrc = (CComboBox*)GetDlgItem(IDC_RECSRC);
		if(recsrc->GetCurSel() != CB_ERR) {
			src.recsrc = recsrc->GetCurSel();
		} else {
			src.recsrc = -1;
		}

		GetDlgItem(IDC_CHANNELNAME)->GetWindowTextW(src.chname);
		GetDlgItem(IDC_CHANNELNUM)->GetWindowTextW(src.chnum);

		GetDlgItem(IDC_PNAME)->GetWindowTextW(src.pname);
		GetDlgItem(IDC_PDETAIL)->GetWindowTextW(src.pdetail);
		GetDlgItem(IDC_PEXTEND)->GetWindowTextW(src.pextend);

		src.genre[0] = CrplstoolDlg::GetComboGenre(IDC_GENREL, IDC_GENREM);
		src.genre[1] = CrplstoolDlg::GetComboGenre(IDC_GENREL2, IDC_GENREM2);
		src.genre[2] = CrplstoolDlg::GetComboGenre(IDC_GENREL3, IDC_GENREM3);
	}

	return;
}

bool CrplstoolDlg::ParamModCheck(void)
{
	// データに変更があればTRUEを，変更が無ければFALSEを返す

	CrplstoolDlg::ParamUpdate(true);						// アプリケーションウインドウからデータを取得する

	if(	(src.recyear	!= bak.recyear	) ||
		(src.recmonth	!= bak.recmonth	) ||
		(src.recday		!= bak.recday	) ||
		(src.rechour	!= bak.rechour	) ||
		(src.recmin		!= bak.recmin	) ||
		(src.recsec		!= bak.recsec	) ) return true;
	
	if(src.rectimezone != bak.rectimezone) return true;

	if(	(src.durhour	!= bak.durhour	) ||
		(src.durmin		!= bak.durmin	) ||
		(src.dursec		!= bak.dursec	) ) return true;

	if(	(src.makerid	!= bak.makerid	) ||
		(src.modelcode	!= bak.modelcode) ) return true;

	if(src.recsrc != bak.recsrc)	return true;

	if(	(src.chname		!= bak.chname	) ||
		(src.chnum		!= bak.chnum	) ||
		(src.pname		!= bak.pname	) ||
		(src.pdetail	!= bak.pdetail	) ||
		(src.pextend	!= bak.pextend	) ) return true;

	if( (src.genre[0]	!= bak.genre[0]	) ||
		(src.genre[1]	!= bak.genre[1]	) ||
		(src.genre[2]	!= bak.genre[2]	) ) return true;

	return false;
}

bool CrplstoolDlg::ParamCheck(void)
{
	// 各パラメータが範囲内にあるかチェックし，問題が無ければTRUEを，異常ならFALSEを返す

	if(	(src.recyear < 0)	|| (src.recyear > 9999)	||
		(src.recmonth < 0)	|| (src.recmonth > 12)	||
		(src.recday < 0)	|| (src.recday > 31)	||
		(src.rechour < 0)	|| (src.rechour > 23)	||
		(src.recmin < 0)	|| (src.recmin > 59)	||
		(src.recsec < 0)	|| (src.recsec > 59)	)
	{
		MessageBox(L"録画日時が異常です", NAMESTRING, MB_ICONSTOP | MB_OK);
		return false;
	}

	if(	(src.rectimezone < 0) || (src.rectimezone > 255) )
	{
		MessageBox(L"タイムゾーンが異常です", NAMESTRING, MB_ICONSTOP | MB_OK);
		return false;
	}

	if(	(src.durhour < 0)	|| (src.durhour > 99)	||
		(src.durmin < 0)	|| (src.durmin > 59)	||
		(src.dursec < 0)	|| (src.dursec > 59)	)
	{
		MessageBox(L"録画期間が異常です", NAMESTRING, MB_ICONSTOP | MB_OK);
		return false;
	}

	if(	(src.makerid < 0)	|| (src.makerid > 65535)	||
		(src.modelcode < 0)	|| (src.modelcode > 65535)	)
	{
		MessageBox(L"メーカーIDもしくは機種コードが異常です", NAMESTRING, MB_ICONSTOP | MB_OK);
		return false;
	}

	if( (_wtoi(src.chnum) < 0) || (_wtoi(src.chnum) > 65535) )
	{
		MessageBox(L"チャンネル番号が異常です", NAMESTRING, MB_ICONSTOP | MB_OK);
		return false;
	}

	WCHAR	wstr[CONVBUFSIZE];

	if(src.chname != bak.chname) {
		int32_t	chnamelen = (int32_t)conv_from_unicode(NULL, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.chname), src.chname.GetLength(), opt.bCharSizeFrom);
		if(chnamelen > 20){
			swprintf_s(wstr, CONVBUFSIZE, L"放送局名が長すぎます\n20 byte以下の長さにしてください\n(現在%d byte)", chnamelen);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			return false;
		}
	}

	if(src.pname != bak.pname) {
		int32_t	pnamelen = (int32_t)conv_from_unicode(NULL, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.pname), src.pname.GetLength(), opt.bCharSizeFrom);
		if(pnamelen > 255){
			swprintf_s(wstr, CONVBUFSIZE, L"番組名が長すぎます\n255 byte以下の長さにしてください\n(現在%d byte)", pnamelen);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			return false;
		}
	}

	if( !src.bSonyRpls && (src.pdetail != bak.pdetail) ) {
		int32_t	pdetaillen = (int32_t)conv_from_unicode(NULL, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.pdetail), src.pdetail.GetLength(), opt.bCharSizeFrom);
		if(pdetaillen > 1200) {
			swprintf_s(wstr, CONVBUFSIZE, L"番組内容が長すぎます\n1200 byte以下の長さにしてください\n(現在%d byte)", pdetaillen);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			return false;
		}
	}

	if( src.bSonyRpls && ( (src.pdetail != bak.pdetail) || (src.pextend != bak.pextend) ) ) {

		int32_t	pdetaillen, pextendlen;
		
		if(src.pdetail != bak.pdetail) {
			pdetaillen = (int32_t)conv_from_unicode(NULL, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.pdetail), src.pdetail.GetLength(), opt.bCharSizeFrom);
		} else {
			pdetaillen = src.pdetaillen;
		}

		if(src.pextend != bak.pextend) {
			pextendlen = (int32_t)conv_from_unicode(NULL, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.pextend), src.pextend.GetLength(), opt.bCharSizeFrom);
		} else {
			pextendlen = src.pextendlen;
		}
		
		if( (pdetaillen + pextendlen) > 1200 ) {
			swprintf_s(wstr, CONVBUFSIZE, L"番組内容と内容詳細の合計が長すぎます\n1200 byte以下の長さにしてください\n(現在%d byte)", pdetaillen + pextendlen);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			return false;
		}
	}

	return true;
}

bool CrplstoolDlg::FileReadAndCheck(const WCHAR *filename)
{
	// ファイル名fnameで示される(rpls, TS)ファイルを開いて番組情報を取得する。成功するとTRUEを，失敗するとFALSEを返す

	WCHAR	fname[_MAX_PATH + 1];
	WCHAR	wstr[CONVBUFSIZE];

	if (_wfullpath(fname, filename, _MAX_PATH + 1) == NULL) wcscpy_s(fname, _MAX_PATH + 1, filename);

	src.bReadOnly = false;

	hRplsFile = CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hRplsFile == INVALID_HANDLE_VALUE) {																					// ファイルを開けない場合、リードオンリーで再度開く
		hRplsFile = CreateFile(fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hRplsFile == INVALID_HANDLE_VALUE) {																				// それでも開けない場合はエラー
			swprintf_s(wstr, CONVBUFSIZE, L"ファイル %s を開けませんでした", fname);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			return	false;
		}
		src.bReadOnly = true;
	}

	uint8_t		buf[1024] = { 0 };																								// ファイル種別判断用，読み込みバッファ

	uint32_t	numRead;
	int32_t		tspacketsize;

	SetFilePointer(hRplsFile, 0, NULL, FILE_BEGIN);
	ReadFile(hRplsFile, buf, 1024, (LPDWORD)&numRead, NULL);																	// ファイル先頭をすこし読み込む

	if( (buf[0] == 'P') && (buf[1] == 'L') && (buf[2] == 'S') && (buf[3] == 'T') )												// rplsファイルの場合
	{
		src.filelen = GetFileSize(hRplsFile, NULL);
		if(src.filelen > RPLSFILESIZE) {
			swprintf_s(wstr, CONVBUFSIZE, L"RPLSファイル %s のサイズが大きすぎます", fname);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			CloseHandle(hRplsFile);
			hRplsFile = INVALID_HANDLE_VALUE;
			return	false;
		}

		SetFilePointer(hRplsFile, 0, NULL, FILE_BEGIN);
		ReadFile(hRplsFile, filebuf, (DWORD)src.filelen, (LPDWORD)&numRead, NULL);														// rplsファイルをバッファに読み込む
		if(src.filelen != numRead){
			swprintf_s(wstr, CONVBUFSIZE, L"RPLSファイル %s の読み込みに失敗しました", fname);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			CloseHandle(hRplsFile);
			hRplsFile = INVALID_HANDLE_VALUE;
			return	false;
		}

		src.filename = fname;
		std::tr2::sys::path myfile(fname);
		src.shortfname = myfile.filename().c_str();

		CrplstoolDlg::ParamLoad();																							// rplsデータから番組情報の取得

		if(src.bReadOnly) {																									// リードオンリーの場合、この時点で先にファイルを閉じる
			CloseHandle(hRplsFile);
			hRplsFile = INVALID_HANDLE_VALUE;
		}

		return	true;
	}

	if( (buf[188 * 0] == 0x47) && (buf[188 * 1] == 0x47) && (buf[188 * 2] == 0x47) && (buf[188 * 3] == 0x47) )				// 188バイトパケットTSファイルの場合
	{
		tspacketsize = 188;
	}
	else if( (buf[192 * 0 + 4] == 0x47) && (buf[192 * 1 + 4] == 0x47) && (buf[192 * 2 + 4] == 0x47) && (buf[192 * 3 + 4] == 0x47) )		// 192バイトパケットTSファイルの場合
	{
		tspacketsize = 192;
	}
	else
	{
		swprintf_s(wstr, CONVBUFSIZE, L"ファイル %s は有効なRPLSファイルではありません", fname);
		MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);																	// 不明なファイル
		CloseHandle(hRplsFile);
		hRplsFile = INVALID_HANDLE_VALUE;
		return	false;
	}

	CrplstoolDlg::ParamInit();
	src.bReadOnly = true;																									// TSファイルは編集保存不可なのでリードオンリー扱い

	if(!readTsProgInfo(hRplsFile, &src, tspacketsize, &opt)) {											// TSファイルから番組情報の取得
		swprintf_s(wstr, CONVBUFSIZE, L"TSファイル %s から有効な番組情報を検出できませんでした", fname);
		MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);																	// 取得できなければエラー表示
		CloseHandle(hRplsFile);
		hRplsFile = INVALID_HANDLE_VALUE;
		return	false;
	}

	src.filename = fname;
	std::tr2::sys::path myfile(fname);
	src.shortfname = myfile.filename().c_str();

	bak = src;

	CloseHandle(hRplsFile);																									// ファイル閉じる
	hRplsFile = INVALID_HANDLE_VALUE;
	
	return	true;
}

void CrplstoolDlg::ParamLoad(void)
{
	// rplsファイルデータからの各番組情報データの取得

	src.bSonyRpls	= CrplstoolDlg::ParamMakerCheck(MAKERID_SONY);
	src.bPanaRpls	= CrplstoolDlg::ParamMakerCheck(MAKERID_PANASONIC);

	const uint8_t	*appinfo = filebuf + ADR_APPINFO;
	const uint8_t	*mpdata  = filebuf + (filebuf[ADR_MPDATA] << 24) + (filebuf[ADR_MPDATA + 1] << 16) + (filebuf[ADR_MPDATA + 2] << 8) + filebuf[ADR_MPDATA + 3];

	src.recyear		=	(appinfo[ADR_RECYEAR]  >> 4) * 1000 + (appinfo[ADR_RECYEAR] & 0x0f) * 100 + (appinfo[ADR_RECYEAR + 1] >> 4) * 10 + (appinfo[ADR_RECYEAR + 1] & 0x0f);
	src.recmonth	=	(appinfo[ADR_RECMONTH] >> 4) * 10 + (appinfo[ADR_RECMONTH] & 0x0f);
	src.recday		=	(appinfo[ADR_RECDAY]   >> 4) * 10 + (appinfo[ADR_RECDAY]  & 0x0f);
	src.rechour		=	(appinfo[ADR_RECHOUR]  >> 4) * 10 + (appinfo[ADR_RECHOUR] & 0x0f);
	src.recmin		=	(appinfo[ADR_RECMIN]   >> 4) * 10 + (appinfo[ADR_RECMIN]  & 0x0f);
	src.recsec		=	(appinfo[ADR_RECSEC]   >> 4) * 10 + (appinfo[ADR_RECSEC]  & 0x0f);
	src.durhour		=	(appinfo[ADR_DURHOUR]  >> 4) * 10 + (appinfo[ADR_DURHOUR] & 0x0f);
	src.durmin		=	(appinfo[ADR_DURMIN]   >> 4) * 10 + (appinfo[ADR_DURMIN]  & 0x0f);
	src.dursec		=	(appinfo[ADR_DURSEC]   >> 4) * 10 + (appinfo[ADR_DURSEC]  & 0x0f);
	src.rectimezone	=	appinfo[ADR_TIMEZONE];
	src.makerid		=	appinfo[ADR_MAKERID] * 256 + appinfo[ADR_MAKERID + 1];
	src.modelcode	=	appinfo[ADR_MODELCODE] * 256 + appinfo[ADR_MODELCODE + 1];

	src.chnum.Format(L"%.3d", appinfo[ADR_CHANNELNUM] * 256 + appinfo[ADR_CHANNELNUM + 1]);

	WCHAR	wstr[CONVBUFSIZE];

	int32_t	chnamelen = appinfo[ADR_CHANNELNAME];
	conv_to_unicode((char16_t*)wstr, CONVBUFSIZE, appinfo + ADR_CHANNELNAME + 1, chnamelen, opt.bCharSizeTo, opt.bIVS);
	src.chname = wstr;

	int32_t pnamelen = appinfo[ADR_PNAME];
	conv_to_unicode((char16_t*)wstr, CONVBUFSIZE, appinfo + ADR_PNAME + 1, pnamelen, opt.bCharSizeTo, opt.bIVS);
	src.pname = wstr;

	src.pdetaillen = appinfo[ADR_PDETAIL] * 256 + appinfo[ADR_PDETAIL + 1];
	conv_to_unicode((char16_t*)wstr, CONVBUFSIZE, appinfo + ADR_PDETAIL + 2, src.pdetaillen, opt.bCharSizeTo, opt.bIVS);
	src.pdetail = wstr;

	for(int32_t i = 0; i < 3; i++) src.genre[i] = -1;

	if(src.bSonyRpls) {

		for(int32_t i = 0; i < 3; i++) src.genre[i]	= (mpdata[ADR_GENRE + i * 4 + 0] == 0x01) ? mpdata[ADR_GENRE + i * 4 + 1] : -1;

		src.pextendlen = mpdata[ADR_PEXTENDLEN] * 256 + mpdata[ADR_PEXTENDLEN + 1];
		conv_to_unicode((char16_t*)wstr, CONVBUFSIZE, appinfo + ADR_PDETAIL + 2 + src.pdetaillen, src.pextendlen, opt.bCharSizeTo, opt.bIVS);
		src.pextend = wstr;
	}
	else if(src.bPanaRpls) {

		src.recsrc = GetRecSrcIndex(mpdata[ADR_RECSRC_PANA] * 256 + mpdata[ADR_RECSRC_PANA + 1], true);

		src.pextend = L"";
		src.pextendlen = 0;
		
		switch(mpdata[ADR_GENRE_PANA])
		{
			case 0xD5:
				src.genre[2] = ((mpdata[ADR_GENRE_PANA + 1] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 1] >> 4);
			case 0xE5:
				src.genre[1] = ((mpdata[ADR_GENRE_PANA + 2] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 2] >> 4);
			case 0xE9:
				src.genre[0] = ((mpdata[ADR_GENRE_PANA + 3] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 3] >> 4);
				break;
			default:
				break;
		}
	}
	else {
		src.pextend = L"";
		src.pextendlen = 0;
	}

	bak = src;

	return;
}

bool CrplstoolDlg::ParamMakerCheck(const int32_t idMaker)
{
	// パラメータidMakerで指定されたメーカーIDを有するrplsファイルならTRUEを返す

	const uint32_t	mpadr = (filebuf[ADR_MPDATA] << 24) + (filebuf[ADR_MPDATA + 1] << 16) + (filebuf[ADR_MPDATA + 2] << 8) + filebuf[ADR_MPDATA + 3];	// MakersPrivateData領域のアドレス取得
	if( opt.bForceNormalRpls || (mpadr == 0) ) return false;																							// MakersPrivateData領域が無い場合、通常0になっているはず

	const uint8_t	*mpdata = filebuf + mpadr;
	const int32_t	makerid = (mpdata[ADR_MPMAKERID] << 8) + mpdata[ADR_MPMAKERID + 1];		// MakersPrivateData領域内のメーカーIDを取得する

	if(makerid != idMaker) return false;

	return true;
}

void CrplstoolDlg::OnBnClickedReload()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	// 元に戻すボタンが押された場合

	src = bak;									// バックアップからデータを戻す
	CrplstoolDlg::ParamUpdate(false);

	return;
}

void CrplstoolDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: ここにメッセージ ハンドラ コードを追加するか、既定の処理を呼び出します。
	// ファイルがドラッグ＆ドロップされた場合の処理

	if(hRplsFile != INVALID_HANDLE_VALUE) {
		if(CrplstoolDlg::ParamModCheck()) {
			int	result = MessageBox(L"開いているファイルの変更内容は失われます\n宜しいですか？", NAMESTRING, MB_ICONEXCLAMATION | MB_OKCANCEL);
			if(result != IDOK) {
				CDialog::OnDropFiles(hDropInfo);
				return;
			}
		}
		CloseHandle(hRplsFile);
	}

	WCHAR	fname[_MAX_PATH + 1];

	uint32_t	num = DragQueryFile(hDropInfo, 0xFFFFFFFF, fname, _MAX_PATH);								// ファイル数の取得
	DragQueryFile(hDropInfo, 0, fname, _MAX_PATH);															// ファイル名の取得
	
	if(!CrplstoolDlg::FileReadAndCheck(fname)) CrplstoolDlg::ParamInit();
	CrplstoolDlg::ParamUpdate(false);

	for (uint32_t i = 1; i < num; i++)
	{
		DragQueryFile(hDropInfo, i, fname, _MAX_PATH);														// 2個以上のファイルがDnDされた場合、2つ目以降は別個にアプリケーションを起動する
		CrplstoolDlg::OpenNewAppProcess(fname, i);
		Sleep(100);
	}

	CDialog::OnDropFiles(hDropInfo);
}

void CrplstoolDlg::OnBnClickedCancel()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	// 終了ボタンが押された場合の処理

	if(GetKeyState(VK_ESCAPE) < 0) return;

	if(hRplsFile != INVALID_HANDLE_VALUE) {
		if(CrplstoolDlg::ParamModCheck()) {
			int	result = MessageBox(L"開いているファイルの変更内容は失われます\n宜しいですか？", NAMESTRING, MB_ICONEXCLAMATION | MB_OKCANCEL);
			if(result != IDOK) return;
		}
		CloseHandle(hRplsFile);						// 開いているファイルがあれば閉じる
	}

	delete [] filebuf;
	delete [] savebuf;

	OnCancel();
}

bool CrplstoolDlg::ParamSave(void)
{
	// データをrplsファイルに保存する。成功するとTRUEを，失敗するとFALSEを返す

	memcpy(savebuf, filebuf, RPLSFILESIZE);

	uint8_t	*appinfo = savebuf + ADR_APPINFO;
	uint8_t	*mpdata  = savebuf + (filebuf[ADR_MPDATA] << 24) + (filebuf[ADR_MPDATA + 1] << 16) + (filebuf[ADR_MPDATA + 2] << 8) + filebuf[ADR_MPDATA + 3];

	appinfo[ADR_RECYEAR]		= ((src.recyear / 1000) << 4) + ((src.recyear % 1000) / 100); 
	appinfo[ADR_RECYEAR + 1]	= (((src.recyear % 100) / 10) << 4) + (src.recyear % 10); 
	appinfo[ADR_RECMONTH]		= ((src.recmonth / 10) << 4) + (src.recmonth % 10);
	appinfo[ADR_RECDAY]			= ((src.recday / 10) << 4) + (src.recday % 10);

	appinfo[ADR_RECHOUR]	= ((src.rechour / 10) << 4) + (src.rechour % 10);
	appinfo[ADR_RECMIN]		= ((src.recmin / 10) << 4) + (src.recmin % 10);
	appinfo[ADR_RECSEC]		= ((src.recsec / 10) << 4) + (src.recsec % 10);

	appinfo[ADR_TIMEZONE]	= src.rectimezone;

	appinfo[ADR_DURHOUR]	= ((src.durhour / 10) << 4) + (src.durhour % 10);
	appinfo[ADR_DURMIN]		= ((src.durmin / 10) << 4) + (src.durmin % 10);
	appinfo[ADR_DURSEC]		= ((src.dursec / 10) << 4) + (src.dursec % 10);
	
	appinfo[ADR_MAKERID]		= src.makerid >> 8;
	appinfo[ADR_MAKERID + 1]	= src.makerid & 0x00ff;
	appinfo[ADR_MODELCODE]		= src.modelcode >> 8;
	appinfo[ADR_MODELCODE + 1]	= src.modelcode & 0x00ff;

	if( src.bPanaRpls && (src.recsrc != bak.recsrc) ) {
		int	recsrc = GetRecSrcIndex(src.recsrc, FALSE);
		mpdata[ADR_RECSRC_PANA]		= (recsrc & 0xFF00) >> 8;
		mpdata[ADR_RECSRC_PANA + 1]	= recsrc & 	0x00FF;
	}

	appinfo[ADR_CHANNELNUM]		= _wtoi(src.chnum) >> 8;
	appinfo[ADR_CHANNELNUM + 1]	= _wtoi(src.chnum) & 0x00ff;

	uint8_t	ucstr[CONVBUFSIZE];

	if(src.chname != bak.chname) {
		memset(ucstr, 0, 20);
		uint8_t	chnamelen = (uint8_t)conv_from_unicode(ucstr, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.chname), src.chname.GetLength(), opt.bCharSizeFrom);
		appinfo[ADR_CHANNELNAME] = chnamelen;
		memcpy(appinfo + ADR_CHANNELNAME + 1, ucstr, 20);
	}

	if(src.pname != bak.pname) {
		memset(ucstr, 0, 255);
		uint8_t	pnamelen = (uint8_t)conv_from_unicode(ucstr, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.pname), src.pname.GetLength(), opt.bCharSizeFrom);
		appinfo[ADR_PNAME] = pnamelen;
		memcpy(appinfo + ADR_PNAME + 1, ucstr, 255);
	}

	if( !src.bSonyRpls && (src.pdetail != bak.pdetail) ) {
		memset(ucstr, 0, 1200);
		int32_t	pdetaillen = (int32_t)conv_from_unicode(ucstr, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.pdetail), src.pdetail.GetLength(), opt.bCharSizeFrom);
		appinfo[ADR_PDETAIL]		= (pdetaillen >> 8) & 0xFF;
		appinfo[ADR_PDETAIL + 1]	= pdetaillen & 0xFF;
		memcpy(appinfo + ADR_PDETAIL + 2, ucstr, 1200);
	}

	if( src.bSonyRpls && ( (src.pdetail != bak.pdetail) || (src.pextend != bak.pextend) ) ) {

		int32_t		pdetaillen, pextendlen;

		if(src.pdetail != bak.pdetail) {
			pdetaillen = (int32_t)conv_from_unicode(ucstr, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.pdetail), src.pdetail.GetLength(), opt.bCharSizeFrom);
			appinfo[ADR_PDETAIL]		= (pdetaillen >> 8) & 0xFF;
			appinfo[ADR_PDETAIL + 1]	= pdetaillen & 0xFF;
			memcpy(appinfo + ADR_PDETAIL + 2, ucstr, pdetaillen);
		} else {
			pdetaillen = src.pdetaillen;
		}

		if(src.pextend != bak.pextend) {
			pextendlen = (int32_t)conv_from_unicode(ucstr, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.pextend), src.pextend.GetLength(), opt.bCharSizeFrom);
			mpdata[ADR_PEXTENDLEN]		= (pextendlen >> 8) & 0xFF;
			mpdata[ADR_PEXTENDLEN + 1]	= pextendlen & 0xFF;
			memcpy(appinfo + ADR_PDETAIL + 2 + pdetaillen, ucstr, pextendlen);
		} else {
			pextendlen = src.pextendlen;
			memcpy(appinfo + ADR_PDETAIL + 2 + pdetaillen, filebuf + ADR_APPINFO + ADR_PDETAIL + 2 + src.pdetaillen, pextendlen);
		}

		memset(appinfo + ADR_PDETAIL + 2 + pdetaillen + pextendlen, 0, 1200 - pdetaillen - pextendlen);

		uint32_t	checksum = 0;
		for(int32_t i = 0; i < 1200; i++) checksum += appinfo[ADR_PDETAIL + 2 + i];
		mpdata[ADR_PCHECKSUM] = checksum & 0xff;
	}

	if(src.bSonyRpls) {
		for(int32_t i = 0; i < 3; i++) {
			if(src.genre[i] != bak.genre[i]) {
				mpdata[ADR_GENRE + i * 4 + 0]	= (src.genre[i] != -1) ? 0x01 : 0x00;
				mpdata[ADR_GENRE + i * 4 + 1]	= (src.genre[i] != -1) ? src.genre[i] : 0x00;
			}
		}
	}

	if( src.bPanaRpls && ( ((src.genre[0] != bak.genre[0]) ) || (src.genre[1] != bak.genre[1]) || (src.genre[2] != bak.genre[2]) ) )
	{
		mpdata[ADR_GENRE_PANA + 0]	= 0xEA;

		if(src.genre[0] != -1) {
			mpdata[ADR_GENRE_PANA + 3]	= ((src.genre[0] & 0x0F) << 4) + (src.genre[0] >> 4);
			mpdata[ADR_GENRE_PANA + 0]	= 0xE9;
		} else {
			mpdata[ADR_GENRE_PANA + 3]	= 0xFF;
		}

		if(src.genre[1] != -1) {
			mpdata[ADR_GENRE_PANA + 2]	= ((src.genre[1] & 0x0F) << 4) + (src.genre[1] >> 4);
			mpdata[ADR_GENRE_PANA + 0]	= 0xE5;
		} else {
			mpdata[ADR_GENRE_PANA + 2]	= 0xFF;
		}

		if(src.genre[2] != -1) {
			mpdata[ADR_GENRE_PANA + 1]	= ((src.genre[2] & 0x0F) << 4) + (src.genre[2] >> 4);
			mpdata[ADR_GENRE_PANA + 0]	= 0xD5;
		} else {
			mpdata[ADR_GENRE_PANA + 1]	= 0xFF;
		}
	}

	uint32_t	numWrite;
	SetFilePointer(hRplsFile, 0, NULL, FILE_BEGIN);
	bool	bResult = !!WriteFile(hRplsFile, savebuf, (DWORD)src.filelen, (LPDWORD)&numWrite, NULL);

	if( !bResult || (src.filelen != numWrite) ) return false;

	return true;
}

void CrplstoolDlg::OnBnClickedOk()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	// 保存ボタンが押された場合の処理

	if(GetKeyState(VK_RETURN) < 0) return;

	CrplstoolDlg::ParamUpdate(TRUE);															// アプリケーションウインドウからのデータ取得
	if(!CrplstoolDlg::ParamCheck()) return;														// データが正しいパラメータ範囲内にあるかチェック

	if(!CrplstoolDlg::ParamSave()) {															// ファイルに保存，失敗したらエラー
		MessageBox(L"ファイルの保存に失敗しました", NAMESTRING, MB_ICONSTOP | MB_OK);
		return;
	}

	CloseHandle(hRplsFile);
	hRplsFile = INVALID_HANDLE_VALUE;

	if( (GetKeyState(VK_SHIFT) >= 0) && !opt.bQuickExit) {
		MessageBox(L"変更内容を保存しました", NAMESTRING, MB_ICONINFORMATION | MB_OK);
		CrplstoolDlg::ParamInit();
		CrplstoolDlg::ParamUpdate(false);
		return;
	}

	MessageBeep(MB_ICONINFORMATION);

	delete [] filebuf;
	delete [] savebuf;

	OnOK();
}

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  ここに初期化を追加してください

	GetDlgItem(IDC_TOOLNAME1)->SetWindowTextW(NAMESTRING NAMESTRING3);


	return TRUE;  // return TRUE unless you set the focus to a control
	// 例外 : OCX プロパティ ページは必ず FALSE を返します。
}

void CrplstoolDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: ここにメッセージ ハンドラ コードを追加するか、既定の処理を呼び出します。

	if(initialW != 0) {
		lpMMI->ptMinTrackSize.x = initialW;			// ウインドウを初期サイズより小さくしない
		lpMMI->ptMinTrackSize.y = initialH; 
	}

	CDialog::OnGetMinMaxInfo(lpMMI);
}

void CrplstoolDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: ここにメッセージ ハンドラ コードを追加します。

	if(baseX != 0) {
		GetDlgItem(IDCANCEL)->MoveWindow(cx - MDIVX(57), cy - MDIVY(21), MDIVX(50), MDIVY(14), FALSE);			// ウインドウリサイズ時のボタン等の表示位置指定
		GetDlgItem(IDOK)->MoveWindow(cx - MDIVX(116), cy - MDIVY(21), MDIVX(50), MDIVY(14), FALSE);
		GetDlgItem(IDC_RELOAD)->MoveWindow(MDIVX(7), cy - MDIVY(21), MDIVX(50), MDIVY(14), FALSE);
		GetDlgItem(IDC_FSELECT)->MoveWindow(cx - MDIVX(57), MDIVY(7), MDIVX(50), MDIVY(14), FALSE);
		GetDlgItem(IDC_FNAME)->MoveWindow(MDIVX(7), MDIVY(7),	cx - MDIVX(73), MDIVY(14), FALSE);
		GetDlgItem(IDC_TEXTFINFO)->MoveWindow(MDIVX(7), MDIVY(26), cx - MDIVX(14), cy - MDIVY(55), FALSE);
		GetDlgItem(IDC_PNAME)->MoveWindow(MDIVX(56), MDIVY(141), cx - MDIVX(73), MDIVY(14), FALSE);

		GetDlgItem(IDC_PDETAIL)->MoveWindow(MDIVX(56), MDIVY(166), cx - MDIVX(73), src.bSonyRpls ? MDIVY(36) : cy - MDIVY(260), FALSE);		// bSonyRplsに従って表示サイズ切り替え

		GetDlgItem(IDC_PEXTEND)->MoveWindow(MDIVX(56), MDIVY(211), cx - MDIVX(73), cy - MDIVY(305), FALSE);
		GetDlgItem(IDC_TEXTPGENRE)->MoveWindow(MDIVX(12), cy - MDIVY(85), MDIVX(38), MDIVY(14), FALSE);

		GetDlgItem(IDC_GENREL)->MoveWindow(MDIVX(56), cy - MDIVY(85), MDIVX(118), MDIVY(12), FALSE);
		GetDlgItem(IDC_GENREM)->MoveWindow(MDIVX(182), cy - MDIVY(85), MDIVX(118), MDIVY(12), FALSE);
		GetDlgItem(IDC_GENREL2)->MoveWindow(MDIVX(56), cy - MDIVY(69), MDIVX(118), MDIVY(12), FALSE);
		GetDlgItem(IDC_GENREM2)->MoveWindow(MDIVX(182), cy - MDIVY(69), MDIVX(118), MDIVY(12), FALSE);
		GetDlgItem(IDC_GENREL3)->MoveWindow(MDIVX(56), cy - MDIVY(53), MDIVX(118), MDIVY(12), FALSE);
		GetDlgItem(IDC_GENREM3)->MoveWindow(MDIVX(182), cy - MDIVY(53), MDIVX(118), MDIVY(12), FALSE);

		((CComboBox*)GetDlgItem(IDC_RECSRC))->SetEditSel(-1, -1);
		((CComboBox*)GetDlgItem(IDC_GENREL))->SetEditSel(-1, -1);												// 何故かコンボボックスが強調表示になるので
		((CComboBox*)GetDlgItem(IDC_GENREM))->SetEditSel(-1, -1);												// それを解除する
		((CComboBox*)GetDlgItem(IDC_GENREL2))->SetEditSel(-1, -1);
		((CComboBox*)GetDlgItem(IDC_GENREM2))->SetEditSel(-1, -1);
		((CComboBox*)GetDlgItem(IDC_GENREL3))->SetEditSel(-1, -1);
		((CComboBox*)GetDlgItem(IDC_GENREM3))->SetEditSel(-1, -1);

		Invalidate();
	}

}

void CrplstoolDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: ここにメッセージ ハンドラ コードを追加するか、既定の処理を呼び出します。

	CMenu	menu;
	BOOL	bResult = menu.LoadMenu(IDR_MENU1);
	
	if(bResult) {
		CMenu	*pmenu = menu.GetSubMenu(0);
		if(pmenu != NULL) {
			pmenu->CheckMenuItem(ID__MENU_FNAME,	copyopt.bFName ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->CheckMenuItem(ID__MENU_RECDATE,	copyopt.bRecDate ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->CheckMenuItem(ID__MENU_RECTIME,	copyopt.bRecTime ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->CheckMenuItem(ID__MENU_DURTIME,	copyopt.bDurTime ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->CheckMenuItem(ID__MENU_TIMEZONE,	copyopt.bTimeZone ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->CheckMenuItem(ID__MENU_MAKERID,	copyopt.bMakerID ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->CheckMenuItem(ID__MENU_MODELCODE, copyopt.bModelCode ? MF_CHECKED : MF_UNCHECKED);
			pmenu->EnableMenuItem(ID__MENU_RECSRC,	src.bPanaRpls ? MF_ENABLED : MF_GRAYED);
			pmenu->CheckMenuItem(ID__MENU_RECSRC,	copyopt.bRecSrc ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->CheckMenuItem(ID__MENU_CHNAME,	copyopt.bChName ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->CheckMenuItem(ID__MENU_CHNUM,	copyopt.bChNum ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->CheckMenuItem(ID__MENU_PNAME,	copyopt.bPName ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->CheckMenuItem(ID__MENU_PDETAIL,	copyopt.bPDetail ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->EnableMenuItem(ID__MENU_PEXTEND,	src.bSonyRpls ?	MF_ENABLED : MF_GRAYED);
			pmenu->CheckMenuItem(ID__MENU_PEXTEND,	copyopt.bPExtend ?	MF_CHECKED : MF_UNCHECKED);
			pmenu->EnableMenuItem(ID__MENU_PGENRE,	(src.bSonyRpls || src.bPanaRpls) ?	MF_ENABLED : MF_GRAYED);
			pmenu->CheckMenuItem(ID__MENU_PGENRE,	copyopt.bPGenre ?	MF_CHECKED : MF_UNCHECKED);
			ClientToScreen(&point);
			pmenu->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, this);
		}
	}

	menu.DestroyMenu();

	CDialog::OnRButtonUp(nFlags, point);
}

void CrplstoolDlg::OnMenuTextCopy(UINT nID)
{
	// TODO: ここにコマンド ハンドラ コードを追加します。

	switch(nID)
	{
		case ID__MENU_TEXT:
			CrplstoolDlg::CopyProgInfoToClipboard(FALSE, FALSE);
			break;
		case ID__MENU_ITEMNAME:
			CrplstoolDlg::CopyProgInfoToClipboard(TRUE, FALSE);
			break;
		case ID__MENU_CSV:
			CrplstoolDlg::CopyProgInfoToClipboard(FALSE, TRUE);
		default:
			break;
	}

}

void CrplstoolDlg::CopyProgInfoToClipboard(const bool bItemName, const bool bCSV)
{
	CrplstoolDlg::ParamUpdate(true);

	CString		pinfotext	= L"";
	CString		name		= L"";
	CString		item		= L"";

	bool		bCopy		= false;

	for(int32_t	i = 0; i < 14; i++)
	{
		switch(i)
		{
			case 0:
				name = L"[ファイル名] ";
				item = src.filename;
				bCopy = copyopt.bFName;
				break;
			case 1:
				name = L"[録画日付] ";
				item.Format(L"%.4d/%.2d/%.2d", src.recyear, src.recmonth, src.recday);
				bCopy = copyopt.bRecDate;
				break;
			case 2:
				name = L"[録画時刻] ";
				item.Format(L"%.2d:%.2d:%.2d", src.rechour, src.recmin, src.recsec);
				bCopy = copyopt.bRecTime;
				break;
			case 3:
				name = L"[録画期間] ";
				item.Format(L"%.2d:%.2d:%.2d", src.durhour, src.durmin, src.dursec);
				bCopy = copyopt.bDurTime;
				break;
			case 4:
				name = L"[タイムゾーン] ";
				item.Format(L"%d", src.rectimezone);
				bCopy = copyopt.bTimeZone;
				break;
			case 5:
				name = L"[メーカーID] ";
				item.Format(L"%d", src.makerid);
				bCopy = copyopt.bMakerID;
				break;
			case 6:
				name = L"[メーカー機種コード] ";
				item.Format(L"%d", src.modelcode);
				bCopy = copyopt.bModelCode;
				break;
			case 7:
				name = L"[放送種別] ";
				GetComboStrRecSrc(src.recsrc, item);
				bCopy = copyopt.bRecSrc;
				break;
			case 8:
				name = L"[放送局名] ";
				item = src.chname;
				bCopy = copyopt.bChName;
				break;
			case 9:
				name = L"[チャンネル番号] ";
				item = src.chnum + L"ch";
				bCopy = copyopt.bChNum;
				break;
			case 10:
				name = L"[番組名] ";
				item = src.pname;
				bCopy = copyopt.bPName;
				break;
			case 11:
				name = L"[番組内容]\r\n";
				item = src.pdetail;
				bCopy = copyopt.bPDetail;
				break;
			case 12:
				name = L"[内容詳細]\r\n";
				item = src.pextend;
				bCopy = copyopt.bPExtend;
				break;
			case 13:
				name = L"[番組ジャンル] ";
				if(src.genre[0] != -1)
				{
					CString	strl;
					CString	strm;
					item = L"";
					for(int i = 0; i < 3; i++) {
						if(src.genre[i] == -1) break;
						GetGenreStrL(src.genre[i] >> 4, strl);
						GetGenreStrM(src.genre[i], strm);
						if(i != 0) item += L"　";
						item = item + strl + L"〔" + strm + L"〕";
					}
				}
				else
				{
					item = L"n/a";
				}
				bCopy = copyopt.bPGenre;
				break;	
			default:
				name = L"";
				item = "";
				bCopy = false;
				break;
		}

		if( !src.bPanaRpls && (i == 7) ) bCopy = false;								// !bParaRplsの場合は放送種別情報を出力しない
		if( !src.bSonyRpls && (i == 12) ) bCopy = false;							// !bSonyRplsの場合は内容詳細情報を出力しない
		if( !src.bPanaRpls && !src.bSonyRpls && (i == 13) ) bCopy = false;			// !bSonyRplsかつ!bPanaRplsの場合はジャンル情報を出力しない

		if(bCopy)
		{
			if(bCSV) {
				item.Replace(L"\"", L"\"\"");
				item.Insert(0, L"\"");
				item += L"\",";
			} else {
				if(bItemName) item = name + item;
				item += STR_NEWLINE;
				if ((i == 11) || (i == 12)) item += STR_NEWLINE;					// 番組内容、内容詳細の後は一行余分に改行する
			}

			pinfotext += item;
		}
		
	}

	if(bCSV){
		pinfotext = pinfotext.Left(pinfotext.GetLength() - 1);
		pinfotext += STR_NEWLINE;
	}


	// クリップボードへの出力

	HGLOBAL hMem	= GlobalAlloc(GMEM_FIXED, (pinfotext.GetLength() + 1) * sizeof(WCHAR));
	if(hMem == NULL) {
		MessageBeep(MB_ICONSTOP);
		return;
	}
	lstrcpy((LPTSTR)hMem, (LPCTSTR)pinfotext);

	if(!::OpenClipboard(NULL)) {
		GlobalFree(hMem);
		MessageBeep(MB_ICONSTOP);
		return;
	}
	::EmptyClipboard();
	::SetClipboardData(CF_UNICODETEXT, hMem);
	::CloseClipboard();

	MessageBeep(MB_ICONINFORMATION);

	return;
}

void CrplstoolDlg::OnMenuPItemSelect(UINT nID)
{
	switch(nID)
	{
		case ID__MENU_FNAME:
			copyopt.bFName = !copyopt.bFName;
			break;
		case ID__MENU_RECDATE:
			copyopt.bRecDate = !copyopt.bRecDate;
			break;
		case ID__MENU_RECTIME:
			copyopt.bRecTime = !copyopt.bRecTime;
			break;
		case ID__MENU_DURTIME:
			copyopt.bDurTime = !copyopt.bDurTime;
			break;
		case ID__MENU_TIMEZONE:
			copyopt.bTimeZone = !copyopt.bTimeZone;
			break;
		case ID__MENU_MAKERID:
			copyopt.bMakerID = !copyopt.bMakerID;
			break;
		case ID__MENU_MODELCODE:
			copyopt.bModelCode = !copyopt.bModelCode;
			break;
		case ID__MENU_RECSRC:
			copyopt.bRecSrc = !copyopt.bRecSrc;
			break;
		case ID__MENU_CHNAME:
			copyopt.bChName = !copyopt.bChName;
			break;
		case ID__MENU_CHNUM:
			copyopt.bChNum = !copyopt.bChNum;
			break;
		case ID__MENU_PNAME:
			copyopt.bPName = !copyopt.bPName;
			break;
		case ID__MENU_PDETAIL:
			copyopt.bPDetail = !copyopt.bPDetail;
			break;
		case ID__MENU_PEXTEND:
			copyopt.bPExtend = !copyopt.bPExtend;
			break;
		case ID__MENU_PGENRE:
			copyopt.bPGenre = !copyopt.bPGenre;
			break;
		default:
			break;
	}

}


BOOL CrplstoolDlg::DestroyWindow()
{
	// TODO: ここに特定なコードを追加するか、もしくは基本クラスを呼び出してください。

	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt(TOOLSECTION, BCHARSIZETO,		_BTOINT(opt.bCharSizeTo));
	pApp->WriteProfileInt(TOOLSECTION, BCHARSIZEFROM,	_BTOINT(opt.bCharSizeFrom));
	pApp->WriteProfileInt(TOOLSECTION, BUSEIVS,			_BTOINT(opt.bIVS));
	pApp->WriteProfileInt(TOOLSECTION, BQUICKEXIT,		_BTOINT(opt.bQuickExit));
	pApp->WriteProfileInt(TOOLSECTION, BFORCENORMALRPLS, _BTOINT(opt.bForceNormalRpls));
	pApp->WriteProfileInt(TOOLSECTION, BSHOWMAX,		_BTOINT(opt.bShowMax));
	pApp->WriteProfileInt(TOOLSECTION, NPACKETLIMIT,	opt.packet_limit);
	pApp->WriteProfileInt(TOOLSECTION, NTSFILEPOS,		opt.tsfilepos);

	pApp->WriteProfileString(TOOLSECTION, STRFONTNAME, opt.fontname);
	pApp->WriteProfileInt(TOOLSECTION, NFONTSIZE, opt.fontsize);

	pApp->WriteProfileInt(COPYSECTION, BFNAME,		_BTOINT(copyopt.bFName));
	pApp->WriteProfileInt(COPYSECTION, BRECDATE,	_BTOINT(copyopt.bRecDate));
	pApp->WriteProfileInt(COPYSECTION, BRECTIME,	_BTOINT(copyopt.bRecTime));
	pApp->WriteProfileInt(COPYSECTION, BDURTIME,	_BTOINT(copyopt.bDurTime));
	pApp->WriteProfileInt(COPYSECTION, BTIMEZONE,	_BTOINT(copyopt.bTimeZone));
	pApp->WriteProfileInt(COPYSECTION, BMAKERID,	_BTOINT(copyopt.bMakerID));
	pApp->WriteProfileInt(COPYSECTION, BMODELCODE,	_BTOINT(copyopt.bModelCode));
	pApp->WriteProfileInt(COPYSECTION, BRECSRC,		_BTOINT(copyopt.bRecSrc));
	pApp->WriteProfileInt(COPYSECTION, BCHNAME,		_BTOINT(copyopt.bChName));
	pApp->WriteProfileInt(COPYSECTION, BCHNUM,		_BTOINT(copyopt.bChNum));
	pApp->WriteProfileInt(COPYSECTION, BPNAME,		_BTOINT(copyopt.bPName));
	pApp->WriteProfileInt(COPYSECTION, BPDETAIL,	_BTOINT(copyopt.bPDetail));
	pApp->WriteProfileInt(COPYSECTION, PEXTEND,		_BTOINT(copyopt.bPExtend));
	pApp->WriteProfileInt(COPYSECTION, PGENRE,		_BTOINT(copyopt.bPGenre));

	WINDOWPLACEMENT		wp;
	GetWindowPlacement(&wp);
	pApp->WriteProfileBinary(WINDOWSECTION, WINDOWPOS, (LPBYTE)&wp, sizeof(wp));

	return CDialog::DestroyWindow();
}


void CrplstoolDlg::LoadToolOptions()
{
	CWinApp* pApp = AfxGetApp();

	opt.bCharSizeTo			= !!pApp->GetProfileInt(TOOLSECTION, BCHARSIZETO,	FALSE);						// 8単位符号->Unicode変換時に、英数空白文字に文字サイズ指定(MSZ, NSZ)を反映するか否か．TRUEで反映する
	opt.bCharSizeFrom		= !!pApp->GetProfileInt(TOOLSECTION, BCHARSIZEFROM, TRUE);						// Unicode->8単位符号変換時に、半角英数空白文字に文字サイズ指定(NSZ, MSZ)を付加するか否か．TRUEで付加する
	opt.bIVS				= !!pApp->GetProfileInt(TOOLSECTION, BUSEIVS,		FALSE);						// 8単位符号->Unicode変換時に、異体字セレクタを使用する
	opt.bQuickExit			= !!pApp->GetProfileInt(TOOLSECTION, BQUICKEXIT,	FALSE);						// TRUEにするとファイル保存後、初期状態に戻らずすぐ終了する
	opt.bForceNormalRpls	= !!pApp->GetProfileInt(TOOLSECTION, BFORCENORMALRPLS, FALSE);					// TRUEにすると強制的に非ソニーレコ製rpls用モードで動作する
	opt.bShowMax			= !!pApp->GetProfileInt(TOOLSECTION, BSHOWMAX,		FALSE);						// TRUEにすると全画面表示状態でアプリケーションを開始する

	opt.packet_limit		= (int32_t)pApp->GetProfileInt(TOOLSECTION, NPACKETLIMIT, DEFAULTPACKETLIMIT);	// TSファイルから番組情報を取得する際の、パケット取得の限度値設定
	opt.tsfilepos			= (int32_t)pApp->GetProfileInt(TOOLSECTION, NTSFILEPOS, DEFAULTTSFILEPOS);		// TSファイルから番組情報を取得する位置の指定
	if(opt.tsfilepos > 99) opt.tsfilepos = DEFAULTTSFILEPOS;

	opt.fontname			= pApp->GetProfileString(TOOLSECTION, STRFONTNAME, L"MS UI Gothic");			// エディットボックスに指定するフォント名
	opt.fontsize			= (int32_t)pApp->GetProfileInt(TOOLSECTION, NFONTSIZE, 12);						// エディットボックスに指定するフォントサイズ

	copyopt.bFName			= !!pApp->GetProfileInt(COPYSECTION, BFNAME,     TRUE);							// 右クリックのコピーメニュー各項目
	copyopt.bRecDate		= !!pApp->GetProfileInt(COPYSECTION, BRECDATE,   TRUE);
	copyopt.bRecTime		= !!pApp->GetProfileInt(COPYSECTION, BRECTIME,   TRUE);
	copyopt.bDurTime		= !!pApp->GetProfileInt(COPYSECTION, BDURTIME,   TRUE);
	copyopt.bTimeZone		= !!pApp->GetProfileInt(COPYSECTION, BTIMEZONE,  TRUE);
	copyopt.bMakerID		= !!pApp->GetProfileInt(COPYSECTION, BMAKERID,   TRUE);
	copyopt.bModelCode		= !!pApp->GetProfileInt(COPYSECTION, BMODELCODE, TRUE);
	copyopt.bRecSrc			= !!pApp->GetProfileInt(COPYSECTION, BRECSRC,    TRUE);
	copyopt.bChName			= !!pApp->GetProfileInt(COPYSECTION, BCHNAME,    TRUE);
	copyopt.bChNum			= !!pApp->GetProfileInt(COPYSECTION, BCHNUM,     TRUE);
	copyopt.bPName			= !!pApp->GetProfileInt(COPYSECTION, BPNAME,     TRUE);
	copyopt.bPDetail		= !!pApp->GetProfileInt(COPYSECTION, BPDETAIL,   TRUE);
	copyopt.bPExtend		= !!pApp->GetProfileInt(COPYSECTION, PEXTEND,    TRUE);
	copyopt.bPGenre			= !!pApp->GetProfileInt(COPYSECTION, PGENRE,     TRUE);

	return;
}


void CrplstoolDlg::SetEditBoxFont()
{
	LOGFONT log_font;

	GetDlgItem(IDC_PEXTEND)->GetFont()->GetLogFont(&log_font);
	wcscpy_s(log_font.lfFaceName, 32, (LPCWSTR)opt.fontname);
	
	log_font.lfHeight = -opt.fontsize;
	log_font.lfWidth = 0;
	
	font.CreateFontIndirect(&log_font);

	GetDlgItem(IDC_RECYEAR)->SetFont(&font);																// 各エディットボックス，コンボボックスにフォントを設定する
	GetDlgItem(IDC_RECMONTH)->SetFont(&font);
	GetDlgItem(IDC_RECDAY)->SetFont(&font);
	GetDlgItem(IDC_RECHOUR)->SetFont(&font);
	GetDlgItem(IDC_RECMIN)->SetFont(&font);
	GetDlgItem(IDC_RECSEC)->SetFont(&font);
	GetDlgItem(IDC_DURHOUR)->SetFont(&font);
	GetDlgItem(IDC_DURMIN)->SetFont(&font);
	GetDlgItem(IDC_DURSEC)->SetFont(&font);
	GetDlgItem(IDC_TIMEZONE)->SetFont(&font);
	GetDlgItem(IDC_MAKERID)->SetFont(&font);
	GetDlgItem(IDC_MODELCODE)->SetFont(&font);
	GetDlgItem(IDC_RECSRC)->SetFont(&font);
	GetDlgItem(IDC_CHANNELNAME)->SetFont(&font);
	GetDlgItem(IDC_CHANNELNUM)->SetFont(&font);
	GetDlgItem(IDC_PNAME)->SetFont(&font);
	GetDlgItem(IDC_PDETAIL)->SetFont(&font);
	GetDlgItem(IDC_PEXTEND)->SetFont(&font);
	GetDlgItem(IDC_GENREL)->SetFont(&font);
	GetDlgItem(IDC_GENREM)->SetFont(&font);
	GetDlgItem(IDC_GENREL2)->SetFont(&font);
	GetDlgItem(IDC_GENREM2)->SetFont(&font);
	GetDlgItem(IDC_GENREL3)->SetFont(&font);
	GetDlgItem(IDC_GENREM3)->SetFont(&font);

	return;
}


void CrplstoolDlg::OpenNewAppProcess(const WCHAR* fname, const int32_t offset)
{
	CString		str;
	str.Format(L"\"%s\" " OPT_WINDOW_POS L" %d", fname, offset);

	ShellExecute(m_hWnd, L"open", rplstoolpath, str, NULL, SW_SHOW);

	return;
}