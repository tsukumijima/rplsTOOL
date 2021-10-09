#pragma once

#include <stdint.h>

//

#define		ADR_APPINFO			0x28
#define		ADR_TIMEZONE		0x08
#define		ADR_RECYEAR			0x0a
#define		ADR_RECMONTH		0x0c
#define		ADR_RECDAY			0x0d
#define		ADR_RECHOUR			0x0e
#define		ADR_RECMIN			0x0f
#define		ADR_RECSEC			0x10
#define		ADR_DURHOUR			0x11
#define		ADR_DURMIN			0x12
#define		ADR_DURSEC			0x13
#define		ADR_MAKERID			0x14
#define		ADR_MODELCODE		0x16
#define		ADR_CHANNELNUM		0x18
#define		ADR_CHANNELNAME		0x1b
#define		ADR_PNAME			0x30
#define		ADR_PDETAIL			0x130

#define		ADR_MPDATA			0x10
#define		ADR_MPMAKERID		0x0c
#define		ADR_MPMODELCODE		0x0e
#define		ADR_GENRE			0x1c
#define		ADR_PEXTENDLEN		0x38
#define		ADR_PCHECKSUM		0x3a

#define		ADR_RECMODE_PANA	0x98
#define		ADR_RECSRC_PANA		0xA8
#define		ADR_GENRE_PANA		0xB0
#define		ADR_MAXBITRATE_PANA	0xBC

#define		MAKERID_PANASONIC	0x0103
#define		MAKERID_SONY		0x0108


//

typedef struct {
	int32_t		recyear;
	int32_t		recmonth;
	int32_t		recday;
	int32_t		rechour;
	int32_t		recmin;
	int32_t		recsec;
	int32_t		durhour;
	int32_t		durmin;
	int32_t		dursec;
	int32_t		rectimezone;
	int32_t		makerid;
	int32_t		modelcode;
	int32_t		recsrc;
	CString		chname;
	CString		chnum;
	CString		pname;
	CString		pdetail;
	int32_t		pdetaillen;
	CString		pextend;
	int32_t		pextendlen;
	int32_t		genre[3];
	CString		filename;
	CString		shortfname;
	size_t		filelen;
	bool		bPanaRpls;
	bool		bSonyRpls;
	bool		bReadOnly;
} RplsFileInfo;


typedef struct {
	bool		bFName;
	bool		bRecDate;
	bool		bRecTime;
	bool		bDurTime;
	bool		bTimeZone;
	bool		bMakerID;
	bool		bModelCode;
	bool		bRecSrc;
	bool		bChName;
	bool		bChNum;
	bool		bPName;
	bool		bPDetail;
	bool		bPExtend;
	bool		bPGenre;
} CopyOptions;


typedef struct {
	bool		bQuickExit;
	bool		bCharSizeTo;
	bool		bCharSizeFrom;
	bool		bIVS;
	bool		bForceNormalRpls;
	bool		bShowMax;
	int32_t		tsfilepos;
	int32_t		packet_limit;
	CString		fontname;
	int32_t		fontsize;
} ToolOptions;
