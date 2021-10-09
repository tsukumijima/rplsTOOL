
// rplstool.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CrplstoolApp:
// このクラスの実装については、rplstool.cpp を参照してください。
//

class CrplstoolApp : public CWinAppEx
{
public:
	CrplstoolApp();

// オーバーライド
	public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern CrplstoolApp theApp;