
// rplstoolDlg.h : �w�b�_�[ �t�@�C��
//

#pragma once

#include <stdint.h>
#include "rplsfileinfo.h"


// CrplstoolDlg �_�C�A���O
class CrplstoolDlg : public CDialog
{
// �R���X�g���N�V����
public:
	CrplstoolDlg(CWnd* pParent = NULL);	// �W���R���X�g���N�^

// �_�C�A���O �f�[�^
	enum { IDD = IDD_RPLSTOOL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �T�|�[�g


// ����
protected:
	HICON m_hIcon;

	// �������ꂽ�A���b�Z�[�W���蓖�Ċ֐�
	virtual BOOL OnInitDialog();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	ToolOptions opt;
	uint8_t *filebuf;
	uint8_t *savebuf;
	HANDLE hRplsFile;
	RplsFileInfo src;
	RplsFileInfo bak;
	int32_t baseX;
	int32_t baseY;
	int32_t initialH;
	int32_t initialW;
	CopyOptions copyopt;
	CFont font;
	CString rplstoolpath;
	CBrush brush;
	void SetComboGenre(const int32_t*);
	void SetComboGenreL(const int32_t, const int32_t, const int32_t);
	void GetGenreStrL(const int32_t, CString&);
	void SetComboGenreM(int32_t, int32_t);
	void GetGenreStrM(const int32_t, CString&);
	int32_t GetComboGenre(const int32_t, const int32_t);
	void SetComboRecSrc(const int32_t);
	void GetComboStrRecSrc(const int32_t, CString&);
	int32_t GetRecSrcIndex(const int32_t, const bool);
	void ParamInit(void);
	void ParamUpdate(const bool);
	bool ParamModCheck(void);
	bool ParamCheck(void);
	bool FileReadAndCheck(const WCHAR*);
	void ParamLoad(void);
	bool ParamMakerCheck(const int32_t);
	bool ParamSave(void);
	void CopyProgInfoToClipboard(const bool, const bool);
	void LoadToolOptions(void);
	void SetEditBoxFont(void);
	void OpenNewAppProcess(const WCHAR*, const int32_t);
public:
	afx_msg void OnBnClickedFselect();
	afx_msg void OnCbnSelendokGenrel();
	afx_msg void OnBnClickedReload();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMenuTextCopy(UINT nID);
	afx_msg void OnMenuPItemSelect(UINT nID);
	virtual BOOL DestroyWindow();
	afx_msg void OnCbnSelchangeGenrel2();
	afx_msg void OnCbnSelchangeGenrel3();
};
