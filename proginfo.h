#pragma once

#include "rplsfileinfo.h"

// 定数

#define		MEDIATYPE_TB		0x5442
#define		MEDIATYPE_BS		0x4253
#define		MEDIATYPE_CS		0x4353
#define		MEDIATYPE_UNKNOWN	0xFFFF

#define		PID_PAT				0x0000
#define		PID_SIT				0x001f
#define		PID_EIT				0x0012
#define		PID_SDT				0x0011
#define		PID_INVALID			0xffff


// 番組情報構造体 これは使わずにRplsFileInfoを変わりに使う

/*
typedef struct {
	unsigned char	recdate[4];
	unsigned char	rectime[3];
	unsigned char	durtime[3];
	unsigned char	rectimezone;
	unsigned char	makerid[4];
	unsigned char	chnum[2];
	unsigned char	chname[1 + 20];
	unsigned char	pname[1 + 255];
	unsigned char	pdetail[2 + 1200];
	int				pdetaillen;
	unsigned char	pextend[1200];
	int				pextendlen;
	unsigned char	genre;
} ProgInfo;
*/


// プロトタイプ宣言

bool			readTsProgInfo(HANDLE, RplsFileInfo *, const int32_t, const ToolOptions *);

int32_t			getSitEit(HANDLE, uint8_t*, const int32_t, const int32_t, const int64_t);
bool			getSdt(HANDLE, uint8_t *, const int32_t, const int32_t, const int32_t, const int64_t);
void			mjd_dec(const int32_t, int32_t*, int32_t*, int32_t*);
int32_t			mjd_enc(const int32_t, const int32_t, const int32_t);
int				comparefornidtable(const void*, const void*);
int32_t			getTbChannelNum(const int32_t, const int32_t, int32_t);

void			parseSit(const uint8_t*, RplsFileInfo*, const ToolOptions*);
int32_t			parseEit(const uint8_t*, RplsFileInfo*, const ToolOptions*);
void			parseSdt(const uint8_t*, RplsFileInfo*, const int32_t, const ToolOptions*);