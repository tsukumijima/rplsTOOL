
// rplstoolDlg.cpp : �����t�@�C��
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

#define		MDIVX(x)			MulDiv(x,baseX,4)				// �_�C�A���O�P�ʂ���s�N�Z�����ւ̕ϊ��p
#define		MDIVY(x)			MulDiv(x,baseY,8)
#define		_BTOINT(x)			(x ? 1 : 0)

#define		STR_NEWLINE			L"\r\n"

#define		OPT_WINDOW_POS		L"--set-window-pos"


// �A�v���P�[�V�����̃o�[�W�������Ɏg���� CAboutDlg �_�C�A���O

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �_�C�A���O �f�[�^
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �T�|�[�g

// ����
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


// CrplstoolDlg �_�C�A���O


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


// CrplstoolDlg ���b�Z�[�W �n���h��

BOOL CrplstoolDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// "�o�[�W�������..." ���j���[���V�X�e�� ���j���[�ɒǉ����܂��B

	// IDM_ABOUTBOX �́A�V�X�e�� �R�}���h�͈͓̔��ɂȂ���΂Ȃ�܂���B
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

	// ���̃_�C�A���O�̃A�C�R����ݒ肵�܂��B�A�v���P�[�V�����̃��C�� �E�B���h�E���_�C�A���O�łȂ��ꍇ�A
	//  Framework �́A���̐ݒ�������I�ɍs���܂��B
	SetIcon(m_hIcon, TRUE);			// �傫���A�C�R���̐ݒ�
	SetIcon(m_hIcon, FALSE);		// �������A�C�R���̐ݒ�

	// TODO: �������������ɒǉ����܂��B

	DragAcceptFiles(TRUE);

	RECT	rect = {4, 8, 0, 0};
	MapDialogRect(&rect);				// �_�C�A���O�x�[�X���j�b�g�擾(�E�C���h�E���T�C�Y���̌v�Z�p)
	baseX = rect.left;
	baseY = rect.top;

	GetWindowRect(&rect);				// �E�C���h�E�����T�C�Y�擾
	initialW = rect.right;
	initialH = rect.bottom;

	try {
		filebuf = new uint8_t[RPLSFILESIZE];											// rpls�t�@�C���ǂݍ��݁A�ۑ��p�o�b�t�@������
		savebuf = new uint8_t[RPLSFILESIZE];
	}
	catch(...) {
		MessageBox(L"�A�v���P�[�V�����o�b�t�@�������̊m�ۂɎ��s���܂���", NAMESTRING, MB_ICONSTOP | MB_OK);
		CDialog::OnCancel();															// �������m�ۂɎ��s������I������
		return TRUE;
	}

	SendDlgItemMessage(IDC_CHANNELNAME, EM_LIMITTEXT, (WPARAM)16384,  0L);				// �G�f�B�b�g�{�b�N�X�̃e�L�X�g�T�C�Y���x�w��
	SendDlgItemMessage(IDC_CHANNELNUM , EM_LIMITTEXT, (WPARAM)16384,  0L); 
	SendDlgItemMessage(IDC_PNAME      , EM_LIMITTEXT, (WPARAM)16384,  0L); 
	SendDlgItemMessage(IDC_PDETAIL    , EM_LIMITTEXT, (WPARAM)16384,  0L); 

	hRplsFile	= INVALID_HANDLE_VALUE;

	CrplstoolDlg::LoadToolOptions();													// .ini�t�@�C������̃I�v�V�����ݒ�̓ǂݍ���
	CrplstoolDlg::SetEditBoxFont();														// �e�G�f�B�b�g�{�b�N�X�C�R���{�{�b�N�X�ɑ΂���t�H���g�̎w��			
	CrplstoolDlg::ParamInit();															// �f�[�^������

	int32_t		argn;
	LPWSTR		*args = CommandLineToArgvW(GetCommandLine(), &argn);					// �R�}���h���C��������擾
	rplstoolpath = args[0];																// �A�v���P�[�V�����{�̂̃p�X

	int32_t		offset = 0;
	
	if ((argn == 4) && (wcscmp(args[2], OPT_WINDOW_POS) == 0)) {
		offset = _wtoi(args[3]);														// �E�C���h�E�ʒu�w��I�v�V���������݂���ꍇ�A�I�t�Z�b�g���擾����
		argn = 2;
	}

	if(argn > 1) {
		CrplstoolDlg::FileReadAndCheck(args[1]);										// �t�@�C�����w�肪����Γǂݍ��݁A�f�[�^�擾
	}

	CrplstoolDlg::ParamUpdate(false);													// �A�v���P�[�V�����E�C���h�E�Ƀf�[�^���f

	WINDOWPLACEMENT		*wp;
	UINT				n;

	if(AfxGetApp()->GetProfileBinary(WINDOWSECTION, WINDOWPOS, (LPBYTE*)&wp, &n)) {		// .ini�t�@�C������E�B���h�E�ʒu���擾����
		int32_t  s = GetSystemMetrics(SM_CYCAPTION);									// �^�C�g���o�[�̍������擾����

		wp->rcNormalPosition.left   += (offset * s);
		wp->rcNormalPosition.top    += (offset * s);
		wp->rcNormalPosition.right  += (offset * s);
		wp->rcNormalPosition.bottom += (offset * s);
		SetWindowPlacement(wp);
		
		delete [] wp;
	}

	if(opt.bShowMax) ShowWindow(SW_SHOWMAXIMIZED);								// �E�C���h�E�ő剻

	for (int32_t i = 2; i < argn; i++) {
		CrplstoolDlg::OpenNewAppProcess(args[i], i - 1);						// �����̑Ώۃt�@�C�����w�肳��Ă���ꍇ�A2�ڈȍ~�͕ʌɃA�v���P�[�V�������N������
		Sleep(100);
	}

	LocalFree(args);															// CommandLineToArgvW()�Ŋm�ۂ��ꂽ�������̊J��

	return TRUE;  // �t�H�[�J�X���R���g���[���ɐݒ肵���ꍇ�������ATRUE ��Ԃ��܂��B
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

// �_�C�A���O�ɍŏ����{�^����ǉ�����ꍇ�A�A�C�R����`�悷�邽�߂�
//  ���̃R�[�h���K�v�ł��B�h�L�������g/�r���[ ���f�����g�� MFC �A�v���P�[�V�����̏ꍇ�A
//  ����́AFramework �ɂ���Ď����I�ɐݒ肳��܂��B

void CrplstoolDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �`��̃f�o�C�X �R���e�L�X�g

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// �N���C�A���g�̎l�p�`�̈���̒���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �A�C�R���̕`��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ���[�U�[���ŏ��������E�B���h�E���h���b�O���Ă���Ƃ��ɕ\������J�[�\�����擾���邽�߂ɁA
//  �V�X�e�������̊֐����Ăяo���܂��B
HCURSOR CrplstoolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CrplstoolDlg::OnBnClickedFselect()
{
	// TODO: �����ɃR���g���[���ʒm�n���h�� �R�[�h��ǉ����܂��B
	// �t�@�C���I���{�^���������ꂽ�ꍇ�̏���

	if( (hRplsFile != INVALID_HANDLE_VALUE) && CrplstoolDlg::ParamModCheck() ) {
		int32_t	result = MessageBox(L"�J���Ă���t�@�C���̕ύX���e�͎����܂�\n�X�����ł����H", NAMESTRING, MB_ICONEXCLAMATION | MB_OKCANCEL);
		if(result != IDOK) return;														// �m�F�_�C�A���O�ŃL�����Z�����ꂽ�璆�~���Ė߂�
	}

	CString		ffilter(L"rpls files (*.rpls)|*.rpls|all files (*.*)|*.*||");
	CFileDialog	fSelDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, ffilter);

	if( fSelDlg.DoModal() != IDOK ) return;												// �t�@�C���I���_�C�A���O�ŃL�����Z�����ꂽ��߂�
	CString		fname = fSelDlg.GetPathName();											// �t�@�C�����擾

	if(hRplsFile != INVALID_HANDLE_VALUE) CloseHandle(hRplsFile);						// �J���Ă���t�@�C���������������

	if( !CrplstoolDlg::FileReadAndCheck((LPCWSTR)fname) ) CrplstoolDlg::ParamInit();	// �t�@�C����ǂݍ���Ńf�[�^�擾�A���s�����珉����
	CrplstoolDlg::ParamUpdate(false);

	return;
}


void CrplstoolDlg::SetComboGenre(const int32_t* genre)
{
	CrplstoolDlg::SetComboGenreL(IDC_GENREL, IDC_GENREM, genre[0]);

	GetDlgItem(IDC_GENREL)->EnableWindow(src.bSonyRpls || src.bPanaRpls);																				// !bSonyRpls����!bPanaRpls�Ŗ����\��
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
	// �ԑg�W���������i�啪�ށC�����ށj�̃R���{�{�b�N�X�\���ݒ�

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

	CrplstoolDlg::SetComboGenreM(idGenreM, genre);	// �����ނ̐ݒ�

	return;
}


void CrplstoolDlg::GetGenreStrL(const int32_t genre, CString& str)
{
	WCHAR	*str_genreL[] = {
		L"�j���[�X�^��",			L"�X�|�[�c",	L"���^���C�h�V���[",	L"�h���}",
		L"���y",					L"�o���G�e�B",	L"�f��",				L"�A�j���^���B",
		L"�h�L�������^���[�^���{",	L"����^����",	L"��^����",			L"����",
		L"�\��",					L"�\��",		L"�g��",				L"���̑�"
	};

	str.Format(L"%s", str_genreL[genre]);

	return;
}


void CrplstoolDlg::SetComboGenreM(int32_t idGenreM, int32_t genre)
{
	// �ԑg�W���������i�����ށj�̃R���{�{�b�N�X�ݒ�

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
		L"�莞�E����", L"�V�C", L"���W�E�h�L�������g", L"�����E����", L"�o�ρE�s��", L"�C�O�E����", L"���", L"���_�E��k",
		L"�񓹓���", L"���[�J���E�n��", L"���", L"-", L"-", L"-", L"-", L"���̑�",

		L"�X�|�[�c�j���[�X", L"�싅", L"�T�b�J�[", L"�S���t", L"���̑��̋��Z", L"���o�E�i���Z", L"�I�����s�b�N�E���ۑ��", L"�}���\���E����E���j",
		L"���[�^�[�X�|�[�c", L"�}�����E�E�B���^�[�X�|�[�c", L"���n�E���c���Z", L"-", L"-", L"-", L"-", L"���̑�",

		L"�|�\�E���C�h�V���[", L"�t�@�b�V����", L"��炵�E�Z�܂�", L"���N�E���", L"�V���b�s���O�E�ʔ�", L"�O�����E����", L"�C�x���g", L"�ԑg�Љ�E���m�点",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�����h���}", L"�C�O�h���}", L"���㌀", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�������b�N�E�|�b�v�X", L"�C�O���b�N�E�|�b�v�X", L"�N���V�b�N�E�I�y��", L"�W���Y�E�t���[�W����", L"�̗w�ȁE����", L"���C�u�E�R���T�[�g", L"�����L���O�E���N�G�X�g", L"�J���I�P�E�̂ǎ���",
		L"���w�E�M�y", L"���w�E�L�b�Y", L"�������y�E���[���h�~���[�W�b�N", L"-", L"-", L"-", L"-", L"���̑�",

		L"�N�C�Y", L"�Q�[��", L"�g�[�N�o���G�e�B", L"���΂��E�R���f�B", L"���y�o���G�e�B", L"���o���G�e�B", L"�����o���G�e�B", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�m��", L"�M��", L"�A�j��", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�����A�j��", L"�C�O�A�j��", L"���B", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�Љ�E����", L"���j�E�I�s", L"���R�E�����E��", L"�F���E�Ȋw�E��w", L"�J���`���[�E�`���|�\", L"���w�E���|", L"�X�|�[�c", L"�h�L�������^���[�S��",
		L"�C���^�r���[�E���_", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"���㌀�E�V��", L"�~���[�W�J��", L"�_���X�E�o���G", L"����E���|", L"�̕���E�ÓT", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"���E�ނ�E�A�E�g�h�A", L"���|�E�y�b�g�E��|", L"���y�E���p�E�H�|", L"�͌�E����", L"�����E�p�`���R", L"�ԁE�I�[�g�o�C", L"�R���s���[�^�E�s�u�Q�[��", L"��b�E��w",
		L"�c���E���w��", L"���w���E���Z��", L"��w���E��", L"���U����E���i", L"������", L"-", L"-", L"���̑�",

		L"�����", L"��Q��", L"�Љ��", L"�{�����e�B�A", L"��b", L"�����i�����j", L"�������", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"BS/�n��f�W�^�������p�ԑg�t�����", L"�L�ш�CS�f�W�^�������p�g��", L"�q���f�W�^�����������p�g��", L"�T�[�o�[�^�ԑg�t�����", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�"
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
	// TODO: �����ɃR���g���[���ʒm�n���h�� �R�[�h��ǉ����܂��B
	// �ԑg�W���������啪�ނɑI��ύX���������ꍇ�A�����ނ��Đݒ肷��

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
	// TODO: �����ɃR���g���[���ʒm�n���h�� �R�[�h��ǉ����܂��B
	// �ԑg�W���������啪�ނɑI��ύX���������ꍇ�A�����ނ��Đݒ肷��

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
	// TODO: �����ɃR���g���[���ʒm�n���h�� �R�[�h��ǉ����܂��B
	// �ԑg�W���������啪�ނɑI��ύX���������ꍇ�A�����ނ��Đݒ肷��

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
	// ������ʁi�^��\�[�X�j�̃R���{�{�b�N�X�\���ݒ�

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
		L"�n��f�W�^��",
		L"BS�f�W�^��",
		L"CS�f�W�^��1",
		L"CS�f�W�^��2",
		L"i.LINK(TS)",
		L"AVCHD",
		L"�X�J�p�[(DLNA)",
		L"DV����",
		L"�n��A�i���O",
		L"���C������",
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
	// bDir:true		����num��recsrc�l�Ƃ݂Ȃ���index�l��Ԃ�
	// bDir:false		����num��index�l�Ƃ݂Ȃ���recsrc�l��Ԃ�

	int32_t		recsrc_table[] =
	{								// ������GetComboStrRecSrc�ƑΉ�����K�v����
		0x5444,						// TD	�n��f�W�^��
		0x4244,						// BD	BS�f�W�^��
		0x4331,						// C1	CS�f�W�^��1
		0x4332,						// C2	CS�f�W�^��2
		0x694C,						// iL	i.LINK(TS)����
		0x4D56,						// MV	AVCHD
		0x534B,						// SK	�X�J�p�[(DLNA)
		0x4456,						// DV	DV����
		0x5441,						// TA	�n��A�i���O
		0x4E4C,						// NL	���C������
	};

	if(!bDir){
		if( (num >= 0) && (num < (sizeof(recsrc_table) / sizeof(int))) ) {
			return	recsrc_table[num];
		} else {
			return -1;
		}
	}

	// �����ȃe�[�u���Ȃ̂ŏ����T����

	for(int32_t i = 0; i < (sizeof(recsrc_table) / sizeof(int32_t)); i++) {
		if(num == recsrc_table[i]) return i;
	}

	return -1;
}


void CrplstoolDlg::ParamInit(void)
{
	// �S�p�����[�^�̏�����

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
	// �A�v���P�[�V�����E�C���h�E�ւ̃f�[�^���f�C�������̓A�v���P�[�V�����E�C���h�E����̃f�[�^�擾

	WCHAR	str[32];

	if(!bDir) {																	// �f�[�^���f

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

		GetDlgItem(IDC_TEXTRECSRC)->ShowWindow(src.bPanaRpls ? SW_SHOW : SW_HIDE);																			// bPanaRpls�ɏ]���āu������ʁv��\�����邩�ۂ����߂�
		GetDlgItem(IDC_RECSRC)->ShowWindow(src.bPanaRpls ? SW_SHOW : SW_HIDE);

		RECT	rect;
		GetClientRect(&rect);
		GetDlgItem(IDC_PDETAIL)->MoveWindow(MDIVX(56), MDIVY(166), rect.right - MDIVX(73), src.bSonyRpls ? MDIVY(36) : rect.bottom - MDIVY(260), TRUE);		// bSonyRpls�ɏ]���āu�ԑg���e�v���̕\���T�C�Y��ύX����

		GetDlgItem(IDC_PEXTEND)->ShowWindow(src.bSonyRpls ? SW_SHOW : SW_HIDE);																				// !bSonyRpls�Ŕ�\��
		GetDlgItem(IDC_TEXTPEXTEND)->ShowWindow(src.bSonyRpls ? SW_SHOW : SW_HIDE);

		GetDlgItem(IDOK)->EnableWindow(hRplsFile != INVALID_HANDLE_VALUE);

	} else {																	// �f�[�^�擾

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
	// �f�[�^�ɕύX�������TRUE���C�ύX���������FALSE��Ԃ�

	CrplstoolDlg::ParamUpdate(true);						// �A�v���P�[�V�����E�C���h�E����f�[�^���擾����

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
	// �e�p�����[�^���͈͓��ɂ��邩�`�F�b�N���C��肪�������TRUE���C�ُ�Ȃ�FALSE��Ԃ�

	if(	(src.recyear < 0)	|| (src.recyear > 9999)	||
		(src.recmonth < 0)	|| (src.recmonth > 12)	||
		(src.recday < 0)	|| (src.recday > 31)	||
		(src.rechour < 0)	|| (src.rechour > 23)	||
		(src.recmin < 0)	|| (src.recmin > 59)	||
		(src.recsec < 0)	|| (src.recsec > 59)	)
	{
		MessageBox(L"�^��������ُ�ł�", NAMESTRING, MB_ICONSTOP | MB_OK);
		return false;
	}

	if(	(src.rectimezone < 0) || (src.rectimezone > 255) )
	{
		MessageBox(L"�^�C���]�[�����ُ�ł�", NAMESTRING, MB_ICONSTOP | MB_OK);
		return false;
	}

	if(	(src.durhour < 0)	|| (src.durhour > 99)	||
		(src.durmin < 0)	|| (src.durmin > 59)	||
		(src.dursec < 0)	|| (src.dursec > 59)	)
	{
		MessageBox(L"�^����Ԃ��ُ�ł�", NAMESTRING, MB_ICONSTOP | MB_OK);
		return false;
	}

	if(	(src.makerid < 0)	|| (src.makerid > 65535)	||
		(src.modelcode < 0)	|| (src.modelcode > 65535)	)
	{
		MessageBox(L"���[�J�[ID�������͋@��R�[�h���ُ�ł�", NAMESTRING, MB_ICONSTOP | MB_OK);
		return false;
	}

	if( (_wtoi(src.chnum) < 0) || (_wtoi(src.chnum) > 65535) )
	{
		MessageBox(L"�`�����l���ԍ����ُ�ł�", NAMESTRING, MB_ICONSTOP | MB_OK);
		return false;
	}

	WCHAR	wstr[CONVBUFSIZE];

	if(src.chname != bak.chname) {
		int32_t	chnamelen = (int32_t)conv_from_unicode(NULL, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.chname), src.chname.GetLength(), opt.bCharSizeFrom);
		if(chnamelen > 20){
			swprintf_s(wstr, CONVBUFSIZE, L"�����ǖ����������܂�\n20 byte�ȉ��̒����ɂ��Ă�������\n(����%d byte)", chnamelen);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			return false;
		}
	}

	if(src.pname != bak.pname) {
		int32_t	pnamelen = (int32_t)conv_from_unicode(NULL, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.pname), src.pname.GetLength(), opt.bCharSizeFrom);
		if(pnamelen > 255){
			swprintf_s(wstr, CONVBUFSIZE, L"�ԑg�����������܂�\n255 byte�ȉ��̒����ɂ��Ă�������\n(����%d byte)", pnamelen);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			return false;
		}
	}

	if( !src.bSonyRpls && (src.pdetail != bak.pdetail) ) {
		int32_t	pdetaillen = (int32_t)conv_from_unicode(NULL, CONVBUFSIZE, (char16_t*)((LPCWSTR)src.pdetail), src.pdetail.GetLength(), opt.bCharSizeFrom);
		if(pdetaillen > 1200) {
			swprintf_s(wstr, CONVBUFSIZE, L"�ԑg���e���������܂�\n1200 byte�ȉ��̒����ɂ��Ă�������\n(����%d byte)", pdetaillen);
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
			swprintf_s(wstr, CONVBUFSIZE, L"�ԑg���e�Ɠ��e�ڍׂ̍��v���������܂�\n1200 byte�ȉ��̒����ɂ��Ă�������\n(����%d byte)", pdetaillen + pextendlen);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			return false;
		}
	}

	return true;
}

bool CrplstoolDlg::FileReadAndCheck(const WCHAR *filename)
{
	// �t�@�C����fname�Ŏ������(rpls, TS)�t�@�C�����J���Ĕԑg�����擾����B���������TRUE���C���s�����FALSE��Ԃ�

	WCHAR	fname[_MAX_PATH + 1];
	WCHAR	wstr[CONVBUFSIZE];

	if (_wfullpath(fname, filename, _MAX_PATH + 1) == NULL) wcscpy_s(fname, _MAX_PATH + 1, filename);

	src.bReadOnly = false;

	hRplsFile = CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hRplsFile == INVALID_HANDLE_VALUE) {																					// �t�@�C�����J���Ȃ��ꍇ�A���[�h�I�����[�ōēx�J��
		hRplsFile = CreateFile(fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hRplsFile == INVALID_HANDLE_VALUE) {																				// ����ł��J���Ȃ��ꍇ�̓G���[
			swprintf_s(wstr, CONVBUFSIZE, L"�t�@�C�� %s ���J���܂���ł���", fname);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			return	false;
		}
		src.bReadOnly = true;
	}

	uint8_t		buf[1024] = { 0 };																								// �t�@�C����ʔ��f�p�C�ǂݍ��݃o�b�t�@

	uint32_t	numRead;
	int32_t		tspacketsize;

	SetFilePointer(hRplsFile, 0, NULL, FILE_BEGIN);
	ReadFile(hRplsFile, buf, 1024, (LPDWORD)&numRead, NULL);																	// �t�@�C���擪���������ǂݍ���

	if( (buf[0] == 'P') && (buf[1] == 'L') && (buf[2] == 'S') && (buf[3] == 'T') )												// rpls�t�@�C���̏ꍇ
	{
		src.filelen = GetFileSize(hRplsFile, NULL);
		if(src.filelen > RPLSFILESIZE) {
			swprintf_s(wstr, CONVBUFSIZE, L"RPLS�t�@�C�� %s �̃T�C�Y���傫�����܂�", fname);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			CloseHandle(hRplsFile);
			hRplsFile = INVALID_HANDLE_VALUE;
			return	false;
		}

		SetFilePointer(hRplsFile, 0, NULL, FILE_BEGIN);
		ReadFile(hRplsFile, filebuf, (DWORD)src.filelen, (LPDWORD)&numRead, NULL);														// rpls�t�@�C�����o�b�t�@�ɓǂݍ���
		if(src.filelen != numRead){
			swprintf_s(wstr, CONVBUFSIZE, L"RPLS�t�@�C�� %s �̓ǂݍ��݂Ɏ��s���܂���", fname);
			MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);
			CloseHandle(hRplsFile);
			hRplsFile = INVALID_HANDLE_VALUE;
			return	false;
		}

		src.filename = fname;
		std::filesystem::path myfile(fname);
		src.shortfname = myfile.filename().c_str();

		CrplstoolDlg::ParamLoad();																							// rpls�f�[�^����ԑg���̎擾

		if(src.bReadOnly) {																									// ���[�h�I�����[�̏ꍇ�A���̎��_�Ő�Ƀt�@�C�������
			CloseHandle(hRplsFile);
			hRplsFile = INVALID_HANDLE_VALUE;
		}

		return	true;
	}

	if( (buf[188 * 0] == 0x47) && (buf[188 * 1] == 0x47) && (buf[188 * 2] == 0x47) && (buf[188 * 3] == 0x47) )				// 188�o�C�g�p�P�b�gTS�t�@�C���̏ꍇ
	{
		tspacketsize = 188;
	}
	else if( (buf[192 * 0 + 4] == 0x47) && (buf[192 * 1 + 4] == 0x47) && (buf[192 * 2 + 4] == 0x47) && (buf[192 * 3 + 4] == 0x47) )		// 192�o�C�g�p�P�b�gTS�t�@�C���̏ꍇ
	{
		tspacketsize = 192;
	}
	else
	{
		swprintf_s(wstr, CONVBUFSIZE, L"�t�@�C�� %s �͗L����RPLS�t�@�C���ł͂���܂���", fname);
		MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);																	// �s���ȃt�@�C��
		CloseHandle(hRplsFile);
		hRplsFile = INVALID_HANDLE_VALUE;
		return	false;
	}

	CrplstoolDlg::ParamInit();
	src.bReadOnly = true;																									// TS�t�@�C���͕ҏW�ۑ��s�Ȃ̂Ń��[�h�I�����[����

	if(!readTsProgInfo(hRplsFile, &src, tspacketsize, &opt)) {											// TS�t�@�C������ԑg���̎擾
		swprintf_s(wstr, CONVBUFSIZE, L"TS�t�@�C�� %s ����L���Ȕԑg�������o�ł��܂���ł���", fname);
		MessageBox(wstr, NAMESTRING, MB_ICONSTOP | MB_OK);																	// �擾�ł��Ȃ���΃G���[�\��
		CloseHandle(hRplsFile);
		hRplsFile = INVALID_HANDLE_VALUE;
		return	false;
	}

	src.filename = fname;
	std::filesystem::path myfile(fname);
	src.shortfname = myfile.filename().c_str();

	bak = src;

	CloseHandle(hRplsFile);																									// �t�@�C������
	hRplsFile = INVALID_HANDLE_VALUE;
	
	return	true;
}

void CrplstoolDlg::ParamLoad(void)
{
	// rpls�t�@�C���f�[�^����̊e�ԑg���f�[�^�̎擾

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
	// �p�����[�^idMaker�Ŏw�肳�ꂽ���[�J�[ID��L����rpls�t�@�C���Ȃ�TRUE��Ԃ�

	const uint32_t	mpadr = (filebuf[ADR_MPDATA] << 24) + (filebuf[ADR_MPDATA + 1] << 16) + (filebuf[ADR_MPDATA + 2] << 8) + filebuf[ADR_MPDATA + 3];	// MakersPrivateData�̈�̃A�h���X�擾
	if( opt.bForceNormalRpls || (mpadr == 0) ) return false;																							// MakersPrivateData�̈悪�����ꍇ�A�ʏ�0�ɂȂ��Ă���͂�

	const uint8_t	*mpdata = filebuf + mpadr;
	const int32_t	makerid = (mpdata[ADR_MPMAKERID] << 8) + mpdata[ADR_MPMAKERID + 1];		// MakersPrivateData�̈���̃��[�J�[ID���擾����

	if(makerid != idMaker) return false;

	return true;
}

void CrplstoolDlg::OnBnClickedReload()
{
	// TODO: �����ɃR���g���[���ʒm�n���h�� �R�[�h��ǉ����܂��B
	// ���ɖ߂��{�^���������ꂽ�ꍇ

	src = bak;									// �o�b�N�A�b�v����f�[�^��߂�
	CrplstoolDlg::ParamUpdate(false);

	return;
}

void CrplstoolDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: �����Ƀ��b�Z�[�W �n���h�� �R�[�h��ǉ����邩�A����̏������Ăяo���܂��B
	// �t�@�C�����h���b�O���h���b�v���ꂽ�ꍇ�̏���

	if(hRplsFile != INVALID_HANDLE_VALUE) {
		if(CrplstoolDlg::ParamModCheck()) {
			int	result = MessageBox(L"�J���Ă���t�@�C���̕ύX���e�͎����܂�\n�X�����ł����H", NAMESTRING, MB_ICONEXCLAMATION | MB_OKCANCEL);
			if(result != IDOK) {
				CDialog::OnDropFiles(hDropInfo);
				return;
			}
		}
		CloseHandle(hRplsFile);
	}

	WCHAR	fname[_MAX_PATH + 1];

	uint32_t	num = DragQueryFile(hDropInfo, 0xFFFFFFFF, fname, _MAX_PATH);								// �t�@�C�����̎擾
	DragQueryFile(hDropInfo, 0, fname, _MAX_PATH);															// �t�@�C�����̎擾
	
	if(!CrplstoolDlg::FileReadAndCheck(fname)) CrplstoolDlg::ParamInit();
	CrplstoolDlg::ParamUpdate(false);

	for (uint32_t i = 1; i < num; i++)
	{
		DragQueryFile(hDropInfo, i, fname, _MAX_PATH);														// 2�ȏ�̃t�@�C����DnD���ꂽ�ꍇ�A2�ڈȍ~�͕ʌɃA�v���P�[�V�������N������
		CrplstoolDlg::OpenNewAppProcess(fname, i);
		Sleep(100);
	}

	CDialog::OnDropFiles(hDropInfo);
}

void CrplstoolDlg::OnBnClickedCancel()
{
	// TODO: �����ɃR���g���[���ʒm�n���h�� �R�[�h��ǉ����܂��B
	// �I���{�^���������ꂽ�ꍇ�̏���

	if(GetKeyState(VK_ESCAPE) < 0) return;

	if(hRplsFile != INVALID_HANDLE_VALUE) {
		if(CrplstoolDlg::ParamModCheck()) {
			int	result = MessageBox(L"�J���Ă���t�@�C���̕ύX���e�͎����܂�\n�X�����ł����H", NAMESTRING, MB_ICONEXCLAMATION | MB_OKCANCEL);
			if(result != IDOK) return;
		}
		CloseHandle(hRplsFile);						// �J���Ă���t�@�C��������Ε���
	}

	delete [] filebuf;
	delete [] savebuf;

	OnCancel();
}

bool CrplstoolDlg::ParamSave(void)
{
	// �f�[�^��rpls�t�@�C���ɕۑ�����B���������TRUE���C���s�����FALSE��Ԃ�

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
	// TODO: �����ɃR���g���[���ʒm�n���h�� �R�[�h��ǉ����܂��B
	// �ۑ��{�^���������ꂽ�ꍇ�̏���

	if(GetKeyState(VK_RETURN) < 0) return;

	CrplstoolDlg::ParamUpdate(TRUE);															// �A�v���P�[�V�����E�C���h�E����̃f�[�^�擾
	if(!CrplstoolDlg::ParamCheck()) return;														// �f�[�^���������p�����[�^�͈͓��ɂ��邩�`�F�b�N

	if(!CrplstoolDlg::ParamSave()) {															// �t�@�C���ɕۑ��C���s������G���[
		MessageBox(L"�t�@�C���̕ۑ��Ɏ��s���܂���", NAMESTRING, MB_ICONSTOP | MB_OK);
		return;
	}

	CloseHandle(hRplsFile);
	hRplsFile = INVALID_HANDLE_VALUE;

	if( (GetKeyState(VK_SHIFT) >= 0) && !opt.bQuickExit) {
		MessageBox(L"�ύX���e��ۑ����܂���", NAMESTRING, MB_ICONINFORMATION | MB_OK);
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

	// TODO:  �����ɏ�������ǉ����Ă�������

	GetDlgItem(IDC_TOOLNAME1)->SetWindowTextW(NAMESTRING NAMESTRING3);


	return TRUE;  // return TRUE unless you set the focus to a control
	// ��O : OCX �v���p�e�B �y�[�W�͕K�� FALSE ��Ԃ��܂��B
}

void CrplstoolDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: �����Ƀ��b�Z�[�W �n���h�� �R�[�h��ǉ����邩�A����̏������Ăяo���܂��B

	if(initialW != 0) {
		lpMMI->ptMinTrackSize.x = initialW;			// �E�C���h�E�������T�C�Y��菬�������Ȃ�
		lpMMI->ptMinTrackSize.y = initialH; 
	}

	CDialog::OnGetMinMaxInfo(lpMMI);
}

void CrplstoolDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: �����Ƀ��b�Z�[�W �n���h�� �R�[�h��ǉ����܂��B

	if(baseX != 0) {
		GetDlgItem(IDCANCEL)->MoveWindow(cx - MDIVX(57), cy - MDIVY(21), MDIVX(50), MDIVY(14), FALSE);			// �E�C���h�E���T�C�Y���̃{�^�����̕\���ʒu�w��
		GetDlgItem(IDOK)->MoveWindow(cx - MDIVX(116), cy - MDIVY(21), MDIVX(50), MDIVY(14), FALSE);
		GetDlgItem(IDC_RELOAD)->MoveWindow(MDIVX(7), cy - MDIVY(21), MDIVX(50), MDIVY(14), FALSE);
		GetDlgItem(IDC_FSELECT)->MoveWindow(cx - MDIVX(57), MDIVY(7), MDIVX(50), MDIVY(14), FALSE);
		GetDlgItem(IDC_FNAME)->MoveWindow(MDIVX(7), MDIVY(7),	cx - MDIVX(73), MDIVY(14), FALSE);
		GetDlgItem(IDC_TEXTFINFO)->MoveWindow(MDIVX(7), MDIVY(26), cx - MDIVX(14), cy - MDIVY(55), FALSE);
		GetDlgItem(IDC_PNAME)->MoveWindow(MDIVX(56), MDIVY(141), cx - MDIVX(73), MDIVY(14), FALSE);

		GetDlgItem(IDC_PDETAIL)->MoveWindow(MDIVX(56), MDIVY(166), cx - MDIVX(73), src.bSonyRpls ? MDIVY(36) : cy - MDIVY(260), FALSE);		// bSonyRpls�ɏ]���ĕ\���T�C�Y�؂�ւ�

		GetDlgItem(IDC_PEXTEND)->MoveWindow(MDIVX(56), MDIVY(211), cx - MDIVX(73), cy - MDIVY(305), FALSE);
		GetDlgItem(IDC_TEXTPGENRE)->MoveWindow(MDIVX(12), cy - MDIVY(85), MDIVX(38), MDIVY(14), FALSE);

		GetDlgItem(IDC_GENREL)->MoveWindow(MDIVX(56), cy - MDIVY(85), MDIVX(118), MDIVY(12), FALSE);
		GetDlgItem(IDC_GENREM)->MoveWindow(MDIVX(182), cy - MDIVY(85), MDIVX(118), MDIVY(12), FALSE);
		GetDlgItem(IDC_GENREL2)->MoveWindow(MDIVX(56), cy - MDIVY(69), MDIVX(118), MDIVY(12), FALSE);
		GetDlgItem(IDC_GENREM2)->MoveWindow(MDIVX(182), cy - MDIVY(69), MDIVX(118), MDIVY(12), FALSE);
		GetDlgItem(IDC_GENREL3)->MoveWindow(MDIVX(56), cy - MDIVY(53), MDIVX(118), MDIVY(12), FALSE);
		GetDlgItem(IDC_GENREM3)->MoveWindow(MDIVX(182), cy - MDIVY(53), MDIVX(118), MDIVY(12), FALSE);

		((CComboBox*)GetDlgItem(IDC_RECSRC))->SetEditSel(-1, -1);
		((CComboBox*)GetDlgItem(IDC_GENREL))->SetEditSel(-1, -1);												// ���̂��R���{�{�b�N�X�������\���ɂȂ�̂�
		((CComboBox*)GetDlgItem(IDC_GENREM))->SetEditSel(-1, -1);												// �������������
		((CComboBox*)GetDlgItem(IDC_GENREL2))->SetEditSel(-1, -1);
		((CComboBox*)GetDlgItem(IDC_GENREM2))->SetEditSel(-1, -1);
		((CComboBox*)GetDlgItem(IDC_GENREL3))->SetEditSel(-1, -1);
		((CComboBox*)GetDlgItem(IDC_GENREM3))->SetEditSel(-1, -1);

		Invalidate();
	}

}

void CrplstoolDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: �����Ƀ��b�Z�[�W �n���h�� �R�[�h��ǉ����邩�A����̏������Ăяo���܂��B

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
	// TODO: �����ɃR�}���h �n���h�� �R�[�h��ǉ����܂��B

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
				name = L"[�t�@�C����] ";
				item = src.filename;
				bCopy = copyopt.bFName;
				break;
			case 1:
				name = L"[�^����t] ";
				item.Format(L"%.4d/%.2d/%.2d", src.recyear, src.recmonth, src.recday);
				bCopy = copyopt.bRecDate;
				break;
			case 2:
				name = L"[�^�掞��] ";
				item.Format(L"%.2d:%.2d:%.2d", src.rechour, src.recmin, src.recsec);
				bCopy = copyopt.bRecTime;
				break;
			case 3:
				name = L"[�^�����] ";
				item.Format(L"%.2d:%.2d:%.2d", src.durhour, src.durmin, src.dursec);
				bCopy = copyopt.bDurTime;
				break;
			case 4:
				name = L"[�^�C���]�[��] ";
				item.Format(L"%d", src.rectimezone);
				bCopy = copyopt.bTimeZone;
				break;
			case 5:
				name = L"[���[�J�[ID] ";
				item.Format(L"%d", src.makerid);
				bCopy = copyopt.bMakerID;
				break;
			case 6:
				name = L"[���[�J�[�@��R�[�h] ";
				item.Format(L"%d", src.modelcode);
				bCopy = copyopt.bModelCode;
				break;
			case 7:
				name = L"[�������] ";
				GetComboStrRecSrc(src.recsrc, item);
				bCopy = copyopt.bRecSrc;
				break;
			case 8:
				name = L"[�����ǖ�] ";
				item = src.chname;
				bCopy = copyopt.bChName;
				break;
			case 9:
				name = L"[�`�����l���ԍ�] ";
				item = src.chnum + L"ch";
				bCopy = copyopt.bChNum;
				break;
			case 10:
				name = L"[�ԑg��] ";
				item = src.pname;
				bCopy = copyopt.bPName;
				break;
			case 11:
				name = L"[�ԑg���e]\r\n";
				item = src.pdetail;
				bCopy = copyopt.bPDetail;
				break;
			case 12:
				name = L"[���e�ڍ�]\r\n";
				item = src.pextend;
				bCopy = copyopt.bPExtend;
				break;
			case 13:
				name = L"[�ԑg�W������] ";
				if(src.genre[0] != -1)
				{
					CString	strl;
					CString	strm;
					item = L"";
					for(int i = 0; i < 3; i++) {
						if(src.genre[i] == -1) break;
						GetGenreStrL(src.genre[i] >> 4, strl);
						GetGenreStrM(src.genre[i], strm);
						if(i != 0) item += L"�@";
						item = item + strl + L"�k" + strm + L"�l";
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

		if( !src.bPanaRpls && (i == 7) ) bCopy = false;								// !bParaRpls�̏ꍇ�͕�����ʏ����o�͂��Ȃ�
		if( !src.bSonyRpls && (i == 12) ) bCopy = false;							// !bSonyRpls�̏ꍇ�͓��e�ڍ׏����o�͂��Ȃ�
		if( !src.bPanaRpls && !src.bSonyRpls && (i == 13) ) bCopy = false;			// !bSonyRpls����!bPanaRpls�̏ꍇ�̓W�����������o�͂��Ȃ�

		if(bCopy)
		{
			if(bCSV) {
				item.Replace(L"\"", L"\"\"");
				item.Insert(0, L"\"");
				item += L"\",";
			} else {
				if(bItemName) item = name + item;
				item += STR_NEWLINE;
				if ((i == 11) || (i == 12)) item += STR_NEWLINE;					// �ԑg���e�A���e�ڍׂ̌�͈�s�]���ɉ��s����
			}

			pinfotext += item;
		}
		
	}

	if(bCSV){
		pinfotext = pinfotext.Left(pinfotext.GetLength() - 1);
		pinfotext += STR_NEWLINE;
	}


	// �N���b�v�{�[�h�ւ̏o��

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
	// TODO: �����ɓ���ȃR�[�h��ǉ����邩�A�������͊�{�N���X���Ăяo���Ă��������B

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

	opt.bCharSizeTo			= !!pApp->GetProfileInt(TOOLSECTION, BCHARSIZETO,	FALSE);						// 8�P�ʕ���->Unicode�ϊ����ɁA�p���󔒕����ɕ����T�C�Y�w��(MSZ, NSZ)�𔽉f���邩�ۂ��DTRUE�Ŕ��f����
	opt.bCharSizeFrom		= !!pApp->GetProfileInt(TOOLSECTION, BCHARSIZEFROM, TRUE);						// Unicode->8�P�ʕ����ϊ����ɁA���p�p���󔒕����ɕ����T�C�Y�w��(NSZ, MSZ)��t�����邩�ۂ��DTRUE�ŕt������
	opt.bIVS				= !!pApp->GetProfileInt(TOOLSECTION, BUSEIVS,		FALSE);						// 8�P�ʕ���->Unicode�ϊ����ɁA�ّ̎��Z���N�^���g�p����
	opt.bQuickExit			= !!pApp->GetProfileInt(TOOLSECTION, BQUICKEXIT,	FALSE);						// TRUE�ɂ���ƃt�@�C���ۑ���A������Ԃɖ߂炸�����I������
	opt.bForceNormalRpls	= !!pApp->GetProfileInt(TOOLSECTION, BFORCENORMALRPLS, FALSE);					// TRUE�ɂ���Ƌ����I�ɔ�\�j�[���R��rpls�p���[�h�œ��삷��
	opt.bShowMax			= !!pApp->GetProfileInt(TOOLSECTION, BSHOWMAX,		FALSE);						// TRUE�ɂ���ƑS��ʕ\����ԂŃA�v���P�[�V�������J�n����

	opt.packet_limit		= (int32_t)pApp->GetProfileInt(TOOLSECTION, NPACKETLIMIT, DEFAULTPACKETLIMIT);	// TS�t�@�C������ԑg�����擾����ۂ́A�p�P�b�g�擾�̌��x�l�ݒ�
	opt.tsfilepos			= (int32_t)pApp->GetProfileInt(TOOLSECTION, NTSFILEPOS, DEFAULTTSFILEPOS);		// TS�t�@�C������ԑg�����擾����ʒu�̎w��
	if(opt.tsfilepos > 99) opt.tsfilepos = DEFAULTTSFILEPOS;

	opt.fontname			= pApp->GetProfileString(TOOLSECTION, STRFONTNAME, L"Meiryo UI");			// �G�f�B�b�g�{�b�N�X�Ɏw�肷��t�H���g��
	opt.fontsize			= (int32_t)pApp->GetProfileInt(TOOLSECTION, NFONTSIZE, 12);						// �G�f�B�b�g�{�b�N�X�Ɏw�肷��t�H���g�T�C�Y

	copyopt.bFName			= !!pApp->GetProfileInt(COPYSECTION, BFNAME,     TRUE);							// �E�N���b�N�̃R�s�[���j���[�e����
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

	GetDlgItem(IDC_RECYEAR)->SetFont(&font);																// �e�G�f�B�b�g�{�b�N�X�C�R���{�{�b�N�X�Ƀt�H���g��ݒ肷��
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