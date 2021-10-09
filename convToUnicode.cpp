#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <memory.h>

#include "convToUnicode.h"
#include "characterSets.h"


//

#ifdef _WIN32
  #define _SNPRINTF(x, len, ...) _snprintf_s(x, len, len - 1, __VA_ARGS__)
#else
  #define _SNPRINTF(x, len, ...) snprintf(x, len, __VA_ARGS__)
#endif

#define		IF_NSZ_TO_MSZ					if(bCharSize&&status.bNormalSize){writeBuf(dbuf,maxbufsize,dst++,0x89);status.bNormalSize=false;}
#define		IF_MSZ_TO_NSZ					if(bCharSize&&!status.bNormalSize){writeBuf(dbuf,maxbufsize,dst++,0x8A);status.bNormalSize=true;}

#define		CHECK_AND_LSHIFT_G0_IN_GL		if(status.region[REGION_GL]!=BANK_G0){WRITEBUF(0x0F);status.region[REGION_GL]=BANK_G0;}
#define		CHECK_AND_LSHIFT_G1_IN_GL		if(status.region[REGION_GL]!=BANK_G1){WRITEBUF(0x0E);status.region[REGION_GL]=BANK_G1;}
#define		CHECK_AND_LSHIFT_G1_IN_GR		if(status.region[REGION_GR]!=BANK_G1){WRITEBUF(0x1B);WRITEBUF(0x7E);status.region[REGION_GR]=BANK_G1;}
#define		CHECK_AND_LSHIFT_G2_IN_GL		if(status.region[REGION_GL]!=BANK_G2){WRITEBUF(0x1B);WRITEBUF(0x6E);status.region[REGION_GL]=BANK_G2;}
#define		CHECK_AND_LSHIFT_G2_IN_GR		if(status.region[REGION_GR]!=BANK_G2){WRITEBUF(0x1B);WRITEBUF(0x7D);status.region[REGION_GR]=BANK_G2;}
#define		CHECK_AND_LSHIFT_G3_IN_GL		if(status.region[REGION_GL]!=BANK_G3){WRITEBUF(0x1B);WRITEBUF(0x6F);status.region[REGION_GL]=BANK_G3;}
#define		CHECK_AND_LSHIFT_G3_IN_GR		if(status.region[REGION_GR]!=BANK_G3){WRITEBUF(0x1B);WRITEBUF(0x7C);status.region[REGION_GR]=BANK_G3;}

#define		SSHIFT_G2_IN_GL					WRITEBUF(0x19)
#define		SSHIFT_G3_IN_GL					WRITEBUF(0x1D)

#define		CHECK_AND_SET_2B_GSET_TO_G0(x)	if(status.bank[BANK_G0]!=x){WRITEBUF(0x1B);WRITEBUF(0x24);WRITEBUF(x);status.bank[BANK_G0]=x;}
#define		CHECK_AND_SET_2B_GSET_TO_G1(x)	if(status.bank[BANK_G1]!=x){WRITEBUF(0x1B);WRITEBUF(0x24);WRITEBUF(0x29);WRITEBUF(x);status.bank[BANK_G1]=x;}
#define		CHECK_AND_SET_2B_GSET_TO_G2(x)	if(status.bank[BANK_G2]!=x){WRITEBUF(0x1B);WRITEBUF(0x24);WRITEBUF(0x2A);WRITEBUF(x);status.bank[BANK_G2]=x;}
#define		CHECK_AND_SET_2B_GSET_TO_G3(x)	if(status.bank[BANK_G3]!=x){WRITEBUF(0x1B);WRITEBUF(0x24);WRITEBUF(0x2B);WRITEBUF(x);status.bank[BANK_G3]=x;}
#define		CHECK_AND_SET_1B_GSET_TO_G0(x)	if(status.bank[BANK_G0]!=x){WRITEBUF(0x1B);WRITEBUF(0x28);WRITEBUF(x);status.bank[BANK_G0]=x;}
#define		CHECK_AND_SET_1B_GSET_TO_G1(x)	if(status.bank[BANK_G1]!=x){WRITEBUF(0x1B);WRITEBUF(0x29);WRITEBUF(x);status.bank[BANK_G1]=x;}
#define		CHECK_AND_SET_1B_GSET_TO_G2(x)	if(status.bank[BANK_G2]!=x){WRITEBUF(0x1B);WRITEBUF(0x2A);WRITEBUF(x);status.bank[BANK_G2]=x;}
#define		CHECK_AND_SET_1B_GSET_TO_G3(x)	if(status.bank[BANK_G3]!=x){WRITEBUF(0x1B);WRITEBUF(0x2B);WRITEBUF(x);status.bank[BANK_G3]=x;}

#define		UTF16BUF(x)						dst+=writeUTF16Buf(dbuf,maxbufsize,dst,x,status.bXCS)
#define		UTF16BUF2(x)					dst+=writeUTF16Buf(dbuf,maxbufsize,dst,x,false)
#define		UTF8BUF(x)						dst+=writeUTF8Buf(dbuf,maxbufsize,dst,x,status.bXCS)
#define		UTF8BUF2(x)						dst+=writeUTF8Buf(dbuf,maxbufsize,dst,x,false)
#define		UINT32TBUF(x,y)					dst+=writeU32TBuf(dbuf,maxbufsize,dst,x,y,&status)
#define		WRITEBUF(x)						writeBuf(dbuf,maxbufsize,dst++,x)

#define		__USE_UTF_CODE_CRLF__			// ���j�R�[�h�o�͂̉��s�� CR+LF ���g�p����B��`���Ȃ���� LF �݂̂ɂȂ�B
#define		__USE_8BITCODE_CR__				// 8�P�ʕ����o�͂̉��s�R�[�h��0x0D���g�p����B��`���Ȃ����0x0A���g�p����


// -----------------------------------------------------------------------------------------------------------------------------------------------

size_t conv_to_unicode(char16_t *dbuf, const size_t maxbufsize, const uint8_t *sbuf, const size_t total_length, const bool bCharSize, const bool bIVS)
{
//		8�P�ʕ��������� -> UNICODE(UTF-16LE)������ւ̕ϊ�
//
//		sbuf				�ϊ���buf
//		total_length		���̒���(uint8_t�P��, NULL���������܂܂Ȃ�)
//		dbuf				�ϊ���buf
//		maxbufsize			�ϊ���buf�̍ő�T�C�Y(char16_t�P��), �z�������͏������܂ꂸ���������
//		bCharSize			�X�y�[�X�y�щp�������̕ϊ��ɕ����T�C�Y�w��(NSZ, MSZ)�𔽉f�����邩�ۂ��Dtrue�Ȃ甽�f������
//		bIVS				���̂̈قȂ銿��(��, ��, �_�Ȃ�)���݂���ꍇ�A���̋�ʂɈّ̎��Z���N�^���g�p����D
//
//		�߂�l				�ϊ����Đ�������UTF-16������̒���(char16_t�P��)
//							dbuf��NULL���w�肷��ƕϊ����Đ�������������̒���(char16_t�P��)�����Ԃ�

	ConvStatus	status;
	initConvStatus(&status);

	size_t		src = 0;
	size_t		dst = 0;

	while(src < total_length)
	{
		if(isControlChar(sbuf[src]))
		{
			// 0x00�`0x20, 0x7F�`0xA0, 0xFF�̏ꍇ

			switch(sbuf[src])
			{
				case 0x08:						// APB (BS)
				case 0x09:						// APF (TAB)
					UTF16BUF(sbuf[src]);												// BS, TAB�o��
					src++;
					break;
				case 0x0A:						// APD (LF)
				case 0x0D:						// APR (CR)
#ifdef __USE_UTF_CODE_CRLF__
					UTF16BUF(0x000D);													// CR�o��
#endif
					UTF16BUF(0x000A);													// LF�o��
					if( (sbuf[src] == 0x0D) && ((src + 1) < total_length) && (sbuf[src + 1] == 0x0A) ) src++;
					src++;
					break;
				case 0x20:						// SP
					UTF16BUF( (bCharSize && status.bNormalSize) ? 0x3000 : 0x0020 );	// �S�p, ���pSP�o��
					src++;
					break;
				case 0x7F:						// DEL
					UTF16BUF(0x007F);													// DEL�o�́D�iDEL�����͑O�i�F�h��Ԃ��Ȃ̂ŁA�{���� U+25A0 �� U+25AE ���o�͂���̂����������삩�Ǝv���܂����A�s���ɂ�肱�����Ă���܂��j
					src++;
					break;
				case 0x9B:						// CSI����
					src += csiProc(sbuf + src, total_length - src, &status);
					break;
				default:						// ����ȊO�̐���R�[�h
					src += changeConvStatus(sbuf + src, total_length - src, &status);
					break;
			}
		}
		else
		{
			// GL, GR�ɑΉ����镶���o��

			int32_t		uc		= 0;
			int32_t		uc2		= 0;
			size_t		len		= 0;
			char16_t	utfStr[UTF16TABLELEN] = { 0 };

			const int32_t	regionLR	= (sbuf[src] >= 0x80) ? REGION_GR : REGION_GL;
			const int32_t	jis			= sbuf[src] & 0x7F;
			const int32_t	jis2		= ( (src + 1) < total_length ) ? (sbuf[src] & 0x7F) * 256 + (sbuf[src + 1] & 0x7F) : 0;

			switch(status.bank[status.region[regionLR]])
			{
				case F_KANJI:
				case F_JIS1KANJI:
					if ((len = jis3CombAndIvsConv(F_JIS1KANJI, jis2, &uc, &uc2)) != 0) {
						UTF16BUF(uc);
						if (bIVS || ((uc2 != 0xE0100) && (uc2 != 0xE0101))) UTF16BUF(uc2);
						src += 2;
						break;
					}
					if (bCharSize && !status.bNormalSize) {															// MSZ�w��̏ꍇ�A���p�����ɕϊ�����
						if (jis2 == 0x2121) uc = 0x20;																// �S�p�󔒂͔��p�󔒂ɕϊ�����
						if (uc == 0) uc = alphaConv(charSize1Conv(jis2, false), true);								// �p�������̏ꍇ�͔��p�����ɕϊ�����
						if (uc == 0) uc = hankaku1Conv(charSize2Conv(jis2, false), true);							// MSZ�w��̃J�^�J�i�����̏ꍇ�͔��p�����ɕϊ�����
					}
					if(uc == 0) uc = jis12Conv(jis2, true);
					if(uc == 0) uc = jis3Conv(jis2, true);
					if(uc != 0) {
                        UTF16BUF(uc);
                    }
					src += 2;
					break;
				case F_JIS2KANJI:
					uc = jis4Conv(jis2, true);
					if(uc != 0) UTF16BUF(uc);
					src += 2;
					break;
				case F_ALPHA:
				case F_P_ALPHA:
					if( bCharSize && status.bNormalSize ) {
						uc = jis12Conv(charSize1Conv(jis, true), true);												// NSZ�w��̏ꍇ�͑S�p�����ɕϊ�����
					} else {
						uc = alphaConv(jis, true);
					}
					if(uc != 0) UTF16BUF(uc);
					src++;
					break;
				case F_HIRAGANA:
				case F_P_HIRAGANA:
					uc = hiragana1Conv(jis, true);
					if(uc != 0) UTF16BUF(uc);
					src++;
					break;
				case F_KATAKANA:
				case F_P_KATAKANA:
					if(bCharSize && !status.bNormalSize ) {
						uc = hankaku1Conv(charSize2Conv(jis12Conv(katakana1Conv(jis, true), false), false), true);	// NSZ�w��̏ꍇ�͔��p�����ɕϊ�����
					} else {
					uc = katakana1Conv(jis, true);
					}
					if(uc != 0) UTF16BUF(uc);
					src++;
					break;
				case F_HANKAKU:
					if( bCharSize && status.bNormalSize ) {
						uc = jis12Conv(charSize2Conv(jis, true), true);												// NSZ�w��̏ꍇ�͑S�p�����ɕϊ�����
					} else {
						uc = hankaku1Conv(jis, true);
					}
					if(uc != 0) UTF16BUF(uc);
					src++;
					break;
				case F_KIGOU:
					if ((len = jis3CombAndIvsConv(F_KIGOU, jis2, &uc, &uc2)) != 0) {
						UTF16BUF(uc);
						if (bIVS) UTF16BUF(uc2);
						src += 2;
						break;
					}
					uc = kigou1Conv(jis2, true);
					if ((uc == 0) && bCharSize && !status.bNormalSize) {
						if (jis2 == 0x2121) uc = 0x20;
						if (uc == 0) uc = alphaConv(charSize1Conv(jis2, false), true);
						if (uc == 0) uc = hankaku1Conv(charSize2Conv(jis2, false), true);							// �ǉ��L���W����1�`84��͖���`���Ǝv���̂ł����A�p�i�����R�ł͂��̕�����
					}																								// �����n�W���Ɠ���̕������o�͂����Ă���Ⴊ�������̂ł������Ă���܂�
					if(uc == 0) uc = jis12Conv(jis2, true);															// ���̏ꍇ�A�\�j�[�����R�ɂ����čs���Ɖ����܂�
					if(uc != 0) {
						UTF16BUF(uc);
					} else {
						len = kigou2ConvUTF16(jis2, utfStr, UTF16TABLELEN);
                        dst += writeUTF16StrBuf(dbuf, maxbufsize, dst, utfStr, len, status.bXCS);
                    }
					src += 2;
					break;
				case F_DRCS0:
					UTF16BUF(0xFF1F);						// '�H'�o��
					src += 2;
					break;
				case F_DRCS1A:
				case F_DRCS2A:
				case F_DRCS3A:
				case F_DRCS4A:
				case F_DRCS5A:
				case F_DRCS6A:
				case F_DRCS7A:
				case F_DRCS8A:
				case F_DRCS9A:
				case F_DRCS10A:
				case F_DRCS11A:
				case F_DRCS12A:
				case F_DRCS13A:
				case F_DRCS14A:
				case F_DRCS15A:
					UTF16BUF(0x003F);						// '?'�o��
					src++;
					break;
				case F_MACROA:
					defaultMacroProc(sbuf[src] & 0x7F, &status);
					src++;
					break;
				case F_MOSAICA:
				case F_MOSAICB:
				case F_MOSAICC:
				case F_MOSAICD:
				default:
					src++;
					break;
			}

			if(status.bSingleShift) {
				status.region[REGION_GL] = status.region_GL_backup;
				status.bSingleShift = false;
			}
		}

	}

	UTF16BUF2(0x0000);

	dst--;
	if(dst > maxbufsize) dst = maxbufsize; 

	return dst;			// �ϊ���̒�����Ԃ�(char16_t�P��), �I�[��null���������܂܂Ȃ�
}


size_t conv_to_unicode(uint8_t *dbuf, const size_t maxbufsize, const uint8_t *sbuf, const size_t total_length, const bool bCharSize, const bool bIVS)
{
//		8�P�ʕ��������� -> UNICODE(UTF-8)������ւ̕ϊ�
//
//		sbuf				�ϊ���buf
//		total_length		���̒���(uint8_t�P��, NULL���������܂܂Ȃ�)
//		dbuf				�ϊ���buf
//		maxbufsize			�ϊ���buf�̍ő�T�C�Y(uint8_t�P��), �z�������͏������܂ꂸ���������
//		bCharSize			�X�y�[�X�y�щp�������̕ϊ��ɕ����T�C�Y�w��(NSZ, MSZ)�𔽉f�����邩�ۂ��Dtrue�Ȃ甽�f������
//		bIVS				���̂̈قȂ銿��(��, ��, �_�Ȃ�)���݂���ꍇ�A���̋�ʂɈّ̎��Z���N�^���g�p����D
//
//		�߂�l				�ϊ����Đ�������UTF-8������̒���(uint8_t�P��)
//							dbuf��NULL���w�肷��ƕϊ����Đ�������������̒���(uint8_t�P��)�����Ԃ�

	ConvStatus	status;
	initConvStatus(&status);

	size_t		src = 0;
	size_t		dst = 0;

	while(src < total_length)
	{
		if(isControlChar(sbuf[src]))
		{
			// 0x00�`0x20, 0x7F�`0xA0, 0xFF�̏ꍇ

			switch(sbuf[src])
			{
				case 0x08:						// APB (BS)
				case 0x09:						// APF (TAB)
					UTF8BUF(sbuf[src]);													// BS, TAB�o��
					src++;
					break;
				case 0x0A:						// APD (LF)
				case 0x0D:						// APR (CR)
#ifdef __USE_UTF_CODE_CRLF__
					UTF8BUF(0x000D);													// CR�o��
#endif
					UTF8BUF(0x000A);													// LF�o��
					if( (sbuf[src] == 0x0D) && ((src + 1) < total_length) && (sbuf[src + 1] == 0x0A) ) src++;
					src++;
					break;
				case 0x20:						// SP
					UTF8BUF( (bCharSize && status.bNormalSize) ? 0x3000 : 0x0020 );	// �S�p, ���pSP�o��
					src++;
					break;
				case 0x7F:						// DEL
					UTF8BUF(0x007F);													// DEL�o�́D�iDEL�����͑O�i�F�h��Ԃ��Ȃ̂ŁA�{���� U+25A0 �� U+25AE ���o�͂���̂����������삩�Ǝv���܂����A�s���ɂ�肱�����Ă���܂��j
					src++;
					break;
				case 0x9B:						// CSI����
					src += csiProc(sbuf + src, total_length - src, &status);
					break;
				default:						// ����ȊO�̐���R�[�h
					src += changeConvStatus(sbuf + src, total_length - src, &status);
					break;
			}
		}
		else
		{
			// GL, GR�ɑΉ����镶���o��

			int32_t		uc		= 0;
			int32_t		uc2		= 0;
			size_t		len		= 0;
			uint8_t		utfStr[UTF8TABLELEN] = { 0 };

			const int32_t	regionLR	= (sbuf[src] >= 0x80) ? REGION_GR : REGION_GL;
			const int32_t	jis			= sbuf[src] & 0x7F;
			const int32_t	jis2		= ( (src + 1) < total_length ) ? (sbuf[src] & 0x7F) * 256 + (sbuf[src + 1] & 0x7F) : 0;

			switch(status.bank[status.region[regionLR]])
			{
				case F_KANJI:
				case F_JIS1KANJI:
					if ((len = jis3CombAndIvsConv(F_JIS1KANJI, jis2, &uc, &uc2)) != 0) {
						UTF8BUF(uc);
						if (bIVS || ((uc2 != 0xE0100) && (uc2 != 0xE0101))) UTF8BUF(uc2);
						src += 2;
						break;
					}
					if (bCharSize && !status.bNormalSize) {															// MSZ�w��̏ꍇ�A���p�����ɕϊ�����
						if (jis2 == 0x2121) uc = 0x20;																// �S�p�󔒂͔��p�󔒂ɕϊ�����
						if (uc == 0) uc = alphaConv(charSize1Conv(jis2, false), true);								// �p�������̏ꍇ�͔��p�����ɕϊ�����
						if (uc == 0) uc = hankaku1Conv(charSize2Conv(jis2, false), true);							// MSZ�w��̃J�^�J�i�����̏ꍇ�͔��p�����ɕϊ�����
					}
					if(uc == 0) uc = jis12Conv(jis2, true);
					if(uc == 0) uc = jis3Conv(jis2, true);
					if(uc != 0) {
                        UTF8BUF(uc);
                    }
					src += 2;
					break;
				case F_JIS2KANJI:
					uc = jis4Conv(jis2, true);
					if(uc != 0) UTF8BUF(uc);
					src += 2;
					break;
				case F_ALPHA:
				case F_P_ALPHA:
					if( bCharSize && status.bNormalSize ) {
						uc = jis12Conv(charSize1Conv(jis, true), true);												// NSZ�w��̏ꍇ�͑S�p�����ɕϊ�����
					} else {
						uc = alphaConv(jis, true);
					}
					if(uc != 0) UTF8BUF(uc);
					src++;
					break;
				case F_HIRAGANA:
				case F_P_HIRAGANA:
					uc = hiragana1Conv(jis, true);
					if(uc != 0) UTF8BUF(uc);
					src++;
					break;
				case F_KATAKANA:
				case F_P_KATAKANA:
					if(bCharSize && !status.bNormalSize ) {
						uc = hankaku1Conv(charSize2Conv(jis12Conv(katakana1Conv(jis, true), false), false), true);	// NSZ�w��̏ꍇ�͔��p�����ɕϊ�����
					} else {
					uc = katakana1Conv(jis, true);
					}
					if(uc != 0) UTF8BUF(uc);
					src++;
					break;
				case F_HANKAKU:
					if( bCharSize && status.bNormalSize ) {
						uc = jis12Conv(charSize2Conv(jis, true), true);												// NSZ�w��̏ꍇ�͑S�p�����ɕϊ�����
					} else {
						uc = hankaku1Conv(jis, true);
					}
					if(uc != 0) UTF8BUF(uc);
					src++;
					break;
				case F_KIGOU:
					if ((len = jis3CombAndIvsConv(F_KIGOU, jis2, &uc, &uc2)) != 0) {
						UTF8BUF(uc);
						if (bIVS) UTF8BUF(uc2);
						src += 2;
						break;
					}
					uc = kigou1Conv(jis2, true);
					if ((uc == 0) && bCharSize && !status.bNormalSize) {
						if (jis2 == 0x2121) uc = 0x20;
						if (uc == 0) uc = alphaConv(charSize1Conv(jis2, false), true);
						if (uc == 0) uc = hankaku1Conv(charSize2Conv(jis2, false), true);							// �ǉ��L���W����1�`84��͖���`���Ǝv���̂ł����A�p�i�����R�ł͂��̕�����
					}																								// �����n�W���Ɠ���̕������o�͂����Ă���Ⴊ�������̂ł������Ă���܂�
					if (uc == 0) uc = jis12Conv(jis2, true);														// ���̏ꍇ�A�\�j�[�����R�ɂ����čs���Ɖ����܂�
					if(uc != 0) {
						UTF8BUF(uc);
					} else {
						len = kigou2ConvUTF8(jis2, utfStr, UTF8TABLELEN);
                        dst += writeUTF8StrBuf(dbuf, maxbufsize, dst, utfStr, len, status.bXCS);
                    }
					src += 2;
					break;
				case F_DRCS0:
					UTF8BUF(0xFF1F);						// '�H'�o��
					src += 2;
					break;
				case F_DRCS1A:
				case F_DRCS2A:
				case F_DRCS3A:
				case F_DRCS4A:
				case F_DRCS5A:
				case F_DRCS6A:
				case F_DRCS7A:
				case F_DRCS8A:
				case F_DRCS9A:
				case F_DRCS10A:
				case F_DRCS11A:
				case F_DRCS12A:
				case F_DRCS13A:
				case F_DRCS14A:
				case F_DRCS15A:
					UTF8BUF(0x003F);						// '?'�o��
					src++;
					break;
				case F_MACROA:
					defaultMacroProc(sbuf[src] & 0x7F, &status);
					src++;
					break;
				case F_MOSAICA:
				case F_MOSAICB:
				case F_MOSAICC:
				case F_MOSAICD:
				default:
					src++;
					break;
			}

			if(status.bSingleShift) {
				status.region[REGION_GL] = status.region_GL_backup;
				status.bSingleShift = false;
			}
		}
	}

	UTF8BUF(0x0000);

	dst--;
	if(dst > maxbufsize) dst = maxbufsize; 

	return dst;			// �ϊ���̒�����Ԃ�(uint8_t�P��), �I�[��null���������܂܂Ȃ�
}


size_t conv_from_unicode(uint8_t *dbuf, const size_t maxbufsize, const char16_t *sbuf, const size_t total_length, const bool bCharSize)
{
	//		UNICODE(UTF-16LE)������ -> 8�P�ʕ���������ւ̕ϊ�
	//
	//		sbuf				�ϊ���buf
	//		total_length		���̒���(char16_t�P��, NULL���������܂܂Ȃ�)
	//		dbuf				�ϊ���buf
	//		maxbufsize			�ϊ���buf�̍ő�T�C�Y(uint8_t�P��), �z�������͏������܂ꂸ���������
	//
	//		�߂�l				�ϊ����Đ�������������̒���(uint8_t�P��)
	//							dbuf��NULL���w�肷��ƕϊ����Đ�������������̒���(uint8_t�P��)�����Ԃ�
	//
	//		�ȉ���BANK���蓖�ĕ��@�ŕϊ��R�X�g�i�ϊ����ʂ�8�P�ʕ�����̒����j���v�Z���A�R�X�g���ł��������Ȃ���@�ŕϊ����s���D
	//
	//		���蓖�ĕ��@ 
	//			REGION_GL, REGION_GR			: BANKG0, BANKG1, BANKG2, BANKG3 �̉�����g�p����
	//			BANKG0, BANKG1, BANKG2, BANKG3 	: F_JIS1KANJI, F_ALPHA, F_HIRAGANA, F_KATAKANA, F_KIGOU, F_HANKAKU,F_JIS2KANJI �̂��ׂĂ̕�����Ɋ��蓖�Ă�D
	//

	ConvStatus	status;
	initConvStatus(&status);

	BankSet		*seq = checkMojiSequenceUTF16(sbuf, total_length, &status);

	size_t		cCount = 0;
	int32_t		cType = status.bank[status.region[REGION_GL]];

	size_t		src = 0;
	size_t		dst = 0;


	// �ϊ����C��

	while (src < total_length)
	{
		int32_t		jisCode;
		size_t		charLen;

		int32_t		charType = classOfCharUTF16(sbuf + src, total_length - src, &jisCode, &charLen);

		switch (charType)
		{
			// ����R�[�h�o�́iSP�܂ށj

		case F_CONTROL:
			switch (jisCode)
			{
				case 0x09:					// TAB
					WRITEBUF(0x09);
					break;
				case 0x0D:
					break;
				case 0x0A:					// LF
#ifdef __USE_8BITCODE_CR__
					WRITEBUF(0x0D);
#else
					WRITEBUF(0x0A);
#endif
					break;
				case 0x20:					// SP
					IF_NSZ_TO_MSZ;
					WRITEBUF(0x20);
					break;
				case 0x7F:					// DEL
					WRITEBUF(0x7F);
					break;
#if 0
				case 0x89:					// MSZ
					WRITEBUF(0x89);
					break;
				case 0x8A:					// NSZ
					WRITEBUF(0x8A);
					break;
#endif
				default:
					break;
			}
			break;

			// �����o��

		case F_ALPHA:
		case F_HANKAKU:
			if (charType != cType) {
				cCount++;													// �����킪�؂�ւ����
				cType = charType;
			}

			IF_NSZ_TO_MSZ;
			switch (seq[cCount])											// seq[]�ɏ]���AG0, G1, G2, G3�̂ǂꂩ�AGL, GR�̂ǂ��炩���g��
			{
				case USE_BANK_G0_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G0(charType);					// F -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G1_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
					WRITEBUF(jisCode);
					break;
				case  USE_BANK_G1_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G2_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G2_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G2_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G3_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G3_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G3_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
					WRITEBUF(jisCode);
					break;
				default:
					break;
			}
			break;

		case F_JIS1KANJI:
		case F_JIS2KANJI:
		case F_KIGOU:
			if (charType != cType) {
				cCount++;													// �����킪�؂�ւ����
				cType = charType;
			}

			IF_MSZ_TO_NSZ;
			switch (seq[cCount])											// seq[]�ɏ]���AG0, G1, G2, G3�̂ǂꂩ�AGL, GR�̂ǂ��炩���g��
			{
				case USE_BANK_G0_TO_GL:
					CHECK_AND_SET_2B_GSET_TO_G0(charType);					// F -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_BANK_G1_TO_GL:
					CHECK_AND_SET_2B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case  USE_BANK_G1_TO_GR:
					CHECK_AND_SET_2B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
					break;
				case USE_BANK_G2_TO_GL:
					CHECK_AND_SET_2B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_BANK_G2_TO_GR:
					CHECK_AND_SET_2B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
				case USE_BANK_G2_SSHIFT:
					CHECK_AND_SET_2B_GSET_TO_G2(charType);					// F -> G2
					SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_BANK_G3_TO_GL:
					CHECK_AND_SET_2B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_BANK_G3_TO_GR:
					CHECK_AND_SET_2B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
				case USE_BANK_G3_SSHIFT:
					CHECK_AND_SET_2B_GSET_TO_G3(charType);					// F -> G3
					SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				default:
					break;
			}
			break;

		case F_HIRAGANA:
		case F_KATAKANA:
		case F_KANACOMMON:
			if (charType != cType) {
				cCount++;													// �����킪�؂�ւ����
				cType = charType;
			}

			IF_MSZ_TO_NSZ;
			switch (seq[cCount])											// seq[]�ɏ]���AG0, G1, G2, G3�̂ǂꂩ�AGL, GR�̂ǂ��炩���g��
			{
				// F_HIRAGANA �́A���̂܂܏o�͂���ꍇ�ƁAF_JIS1KANJI �Ƃ��ďo�͂���ꍇ�̓�̑I����������D
				// F_KATAKANA �́A���̂܂܏o�͂���ꍇ�ƁAF_JIS1KANJI �Ƃ��ďo�͂���ꍇ�̓�̑I����������D
				// F_KANACOMMON �́AF_HIRAGANA, F_KATAKANA, F_JIS1KANJI �Ƃ��ďo�͂���ꍇ�̎O�̑I����������D

				// F_HIRAGANA, F_KATAKANA �����̂܂܏o�͂���ꍇ

				case USE_BANK_G0_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G0(charType);					// F -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G1_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
					WRITEBUF(jisCode);
					break;
				case  USE_BANK_G1_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G2_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G2_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G2_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G3_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G3_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G3_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
					WRITEBUF(jisCode);
					break;

				// F_KANACOMMON �� F_HIRAGANA �Ƃ��ďo�͂���ꍇ

				case USE_HIRAGANA_BANK_G0_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G0(F_HIRAGANA);				// F_HIRAGANA -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
					WRITEBUF(jisCode);
					break;
				case USE_HIRAGANA_BANK_G1_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G1(F_HIRAGANA);				// F_HIRAGANA -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
					WRITEBUF(jisCode);
					break;
				case  USE_HIRAGANA_BANK_G1_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G1(F_HIRAGANA);				// F_HIRAGANA -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_HIRAGANA_BANK_G2_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G2(F_HIRAGANA);				// F_HIRAGANA -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
					WRITEBUF(jisCode);
					break;
				case USE_HIRAGANA_BANK_G2_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G2(F_HIRAGANA);				// F_HIRAGANA -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_HIRAGANA_BANK_G2_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G2(F_HIRAGANA);				// F_HIRAGANA -> G2
					SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
					WRITEBUF(jisCode);
					break;
				case USE_HIRAGANA_BANK_G3_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G3(F_HIRAGANA);				// F_HIRAGANA -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
					WRITEBUF(jisCode);
					break;
				case USE_HIRAGANA_BANK_G3_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G3(F_HIRAGANA);				// F_HIRAGANA -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_HIRAGANA_BANK_G3_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G3(F_HIRAGANA);				// F_HIRAGANA -> G3
					SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
					WRITEBUF(jisCode);
					break;

				// F_KANACOMMON �� F_KATAKANA �Ƃ��ďo�͂���ꍇ

				case USE_KATAKANA_BANK_G0_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G0(F_KATAKANA);				// F_KATAKANA -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
					WRITEBUF(jisCode);
					break;
				case USE_KATAKANA_BANK_G1_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G1(F_KATAKANA);				// F_KATAKANA -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
					WRITEBUF(jisCode);
					break;
				case  USE_KATAKANA_BANK_G1_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G1(F_KATAKANA);				// F_KATAKANA -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_KATAKANA_BANK_G2_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G2(F_KATAKANA);				// F_KATAKANA -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
					WRITEBUF(jisCode);
					break;
				case USE_KATAKANA_BANK_G2_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G2(F_KATAKANA);				// F_KATAKANA -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_KATAKANA_BANK_G2_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G2(F_KATAKANA);				// F_KATAKANA -> G2
					SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
					WRITEBUF(jisCode);
					break;
				case USE_KATAKANA_BANK_G3_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G3(F_KATAKANA);				// F_KATAKANA -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
					WRITEBUF(jisCode);
					break;
				case USE_KATAKANA_BANK_G3_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G3(F_KATAKANA);				// F_KATAKANA -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_KATAKANA_BANK_G3_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G3(F_KATAKANA);				// F_KATAKANA -> G3
					SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
					WRITEBUF(jisCode);
					break;

				// F_HIARGANA, F_KATAKANA, F_KANACOMMON �� F_JIS1KANJI �Ƃ��ďo�͂���ꍇ�D
				
				case USE_KANJI_BANK_G0_TO_GL:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G0(F_JIS1KANJI);							// F_JIS1KANJI -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;											// G0 -> GL (LS0)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_KANJI_BANK_G1_TO_GL:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G1(F_JIS1KANJI);							// F_JIS1KANJI -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;											// G1 -> GL (LS1)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case  USE_KANJI_BANK_G1_TO_GR:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G1(F_JIS1KANJI);							// F_JIS1KANJI -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;											// G1 -> GR (LS1R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
				case USE_KANJI_BANK_G2_TO_GL:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G2(F_JIS1KANJI);							// F_JIS1KANJI -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;											// G2 -> GL (LS2)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_KANJI_BANK_G2_TO_GR:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G2(F_JIS1KANJI);							// F_JIS1KANJI -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;											// G2 -> GR (LS2R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
				case USE_KANJI_BANK_G2_SSHIFT:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G2(F_JIS1KANJI);							// F_JIS1KANJI -> G2
					SSHIFT_G2_IN_GL;													// G2 -> GL (SS2)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_KANJI_BANK_G3_TO_GL:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G3(F_JIS1KANJI);							// F_JIS1KANJI -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;											// G3 -> GL (LS3)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_KANJI_BANK_G3_TO_GR:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G3(F_JIS1KANJI);							// F_JIS1KANJI -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;											// G3 -> GR (LS3R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
				case USE_KANJI_BANK_G3_SSHIFT:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G3(F_JIS1KANJI);							// F_JIS1KANJI -> G3
					SSHIFT_G3_IN_GL;													// G3 -> GL (SS3)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;

				default:
					break;
			}
			break;

		default:
			break;
		}

		src += charLen;
	}

	WRITEBUF(0x00);

	dst--;
	if (dst > maxbufsize) dst = maxbufsize;

	delete[] seq;

	return dst;			// �ϊ���̒�����Ԃ�(uint8_t�P��), �I�[��NULL���������܂܂Ȃ�
}


size_t conv_from_unicode(uint8_t *dbuf, const size_t maxbufsize, const uint8_t *sbuf, const size_t total_length, const bool bCharSize)
{
	//		UNICODE(UTF-8)������ -> 8�P�ʕ���������ւ̕ϊ�
	//
	//		sbuf				�ϊ���buf
	//		total_length		���̒���(uint8_t�P��, NULL���������܂܂Ȃ�)
	//		dbuf				�ϊ���buf
	//		maxbufsize			�ϊ���buf�̍ő�T�C�Y(uint8_t�P��), �z�������͏������܂ꂸ���������
	//
	//		�߂�l				�ϊ����Đ�������������̒���(uint8_t�P��)
	//							dbuf��NULL���w�肷��ƕϊ����Đ�������������̒���(uint8_t�P��)�����Ԃ�
	//
	//		�ȉ���BANK���蓖�ĕ��@�ŕϊ��R�X�g�i�ϊ����ʂ�8�P�ʕ�����̒����j���v�Z���A�R�X�g���ł��������Ȃ���@�ŕϊ����s���D
	//
	//		���蓖�ĕ��@ 
	//			REGION_GL, REGION_GR			: BANKG0, BANKG1, BANKG2, BANKG3 �̉�����g�p����
	//			BANKG0, BANKG1, BANKG2, BANKG3 	: F_JIS1KANJI, F_ALPHA, F_HIRAGANA, F_KATAKANA, F_KIGOU, F_HANKAKU,F_JIS2KANJI �̂��ׂĂ̕�����Ɋ��蓖�Ă�D
	//

	ConvStatus	status;
	initConvStatus(&status);

	BankSet		*seq = checkMojiSequenceUTF8(sbuf, total_length, &status);

	size_t		cCount = 0;
	int32_t		cType = status.bank[status.region[REGION_GL]];

	size_t		src = 0;
	size_t		dst = 0;


	// �ϊ����C��

	while (src < total_length)
	{
		int32_t		jisCode;
		size_t		charLen;

		int32_t		charType = classOfCharUTF8(sbuf + src, total_length - src, &jisCode, &charLen);

		switch (charType)
		{
			// ����R�[�h�o�́iSP�܂ށj

			case F_CONTROL:
				switch (jisCode)
				{
					case 0x09:					// TAB
						WRITEBUF(0x09);
						break;
					case 0x0D:
						break;
					case 0x0A:					// LF
#ifdef __USE_8BITCODE_CR__
						WRITEBUF(0x0D);
#else
						WRITEBUF(0x0A);
#endif
						break;
					case 0x20:					// SP
						IF_NSZ_TO_MSZ;
						WRITEBUF(0x20);
						break;
					case 0x7F:					// DEL
						WRITEBUF(0x7F);
						break;
#if 0
					case 0x89:					// MSZ
						WRITEBUF(0x89);
						break;
					case 0x8A:					// NSZ
						WRITEBUF(0x8A);
						break;
#endif
					default:
						break;
				}
				break;

			// �����o��

			case F_ALPHA:
			case F_HANKAKU:
				if (charType != cType) {
					cCount++;													// �����킪�؂�ւ����
					cType = charType;
				}

				IF_NSZ_TO_MSZ;	
				switch (seq[cCount])											// seq[]�ɏ]���AG0, G1, G2, G3�̂ǂꂩ�AGL, GR�̂ǂ��炩���g��
				{
					case USE_BANK_G0_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G0(charType);					// F -> G0
						CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
						WRITEBUF(jisCode);
						break;
					case USE_BANK_G1_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
						CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
						WRITEBUF(jisCode);
						break;
					case  USE_BANK_G1_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
						CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_BANK_G2_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
						CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
						WRITEBUF(jisCode);
						break;
					case USE_BANK_G2_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
						CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_BANK_G2_SSHIFT:
						CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
						SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
						WRITEBUF(jisCode);
						break;
					case USE_BANK_G3_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
						CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
						WRITEBUF(jisCode);
						break;
					case USE_BANK_G3_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
						CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_BANK_G3_SSHIFT:
						CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
						SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
						WRITEBUF(jisCode);
						break;
					default:
						break;
				}
				break;

			case F_JIS1KANJI:
			case F_JIS2KANJI:
			case F_KIGOU:
				if (charType != cType) {
					cCount++;													// �����킪�؂�ւ����
					cType = charType;
				}

				IF_MSZ_TO_NSZ;
				switch (seq[cCount])											// seq[]�ɏ]���AG0, G1, G2, G3�̂ǂꂩ�AGL, GR�̂ǂ��炩���g��
				{
					case USE_BANK_G0_TO_GL:
						CHECK_AND_SET_2B_GSET_TO_G0(charType);					// F -> G0
						CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					case USE_BANK_G1_TO_GL:
						CHECK_AND_SET_2B_GSET_TO_G1(charType);					// F -> G1
						CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					case  USE_BANK_G1_TO_GR:
						CHECK_AND_SET_2B_GSET_TO_G1(charType);					// F -> G1
						CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
						WRITEBUF((jisCode >> 8) + 0x80);
						WRITEBUF((jisCode & 0xFF) + 0x80);
						break;
						break;
					case USE_BANK_G2_TO_GL:
						CHECK_AND_SET_2B_GSET_TO_G2(charType);					// F -> G2
						CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					case USE_BANK_G2_TO_GR:
						CHECK_AND_SET_2B_GSET_TO_G2(charType);					// F -> G2
						CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
						WRITEBUF((jisCode >> 8) + 0x80);
						WRITEBUF((jisCode & 0xFF) + 0x80);
						break;
					case USE_BANK_G2_SSHIFT:
						CHECK_AND_SET_2B_GSET_TO_G2(charType);					// F -> G2
						SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					case USE_BANK_G3_TO_GL:
						CHECK_AND_SET_2B_GSET_TO_G3(charType);					// F -> G3
						CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					case USE_BANK_G3_TO_GR:
						CHECK_AND_SET_2B_GSET_TO_G3(charType);					// F -> G3
						CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
						WRITEBUF((jisCode >> 8) + 0x80);
						WRITEBUF((jisCode & 0xFF) + 0x80);
						break;
					case USE_BANK_G3_SSHIFT:
						CHECK_AND_SET_2B_GSET_TO_G3(charType);					// F -> G3
						SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					default:
						break;
				}
				break;

			case F_HIRAGANA:
			case F_KATAKANA:
			case F_KANACOMMON:
				if (charType != cType) {
					cCount++;												// �����킪�؂�ւ����
					cType = charType;
				}

				IF_MSZ_TO_NSZ;
				switch (seq[cCount])										// seq[]�ɏ]���AG0, G1, G2, G3�̂ǂꂩ�AGL, GR�̂ǂ��炩���g��
				{
					// F_HIRAGANA �́A���̂܂܏o�͂���ꍇ�ƁAF_JIS1KANJI �Ƃ��ďo�͂���ꍇ�̓�̑I����������D
					// F_KATAKANA �́A���̂܂܏o�͂���ꍇ�ƁAF_JIS1KANJI �Ƃ��ďo�͂���ꍇ�̓�̑I����������D
					// F_KANACOMMON �́AF_HIRAGANA, F_KATAKANA, F_JIS1KANJI �Ƃ��ďo�͂���ꍇ�̎O�̑I����������D

					// F_HIRAGANA, F_KATAKANA �����̂܂܏o�͂���ꍇ

					case USE_BANK_G0_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G0(charType);					// F -> G0
						CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
						WRITEBUF(jisCode);
						break;
					case USE_BANK_G1_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
						CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
						WRITEBUF(jisCode);
						break;
					case  USE_BANK_G1_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
						CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_BANK_G2_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
						CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
						WRITEBUF(jisCode);
						break;
					case USE_BANK_G2_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
						CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_BANK_G2_SSHIFT:
						CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
						SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
						WRITEBUF(jisCode);
						break;
					case USE_BANK_G3_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
						CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
						WRITEBUF(jisCode);
						break;
					case USE_BANK_G3_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
						CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_BANK_G3_SSHIFT:
						CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
						SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
						WRITEBUF(jisCode);
						break;

					// F_KANACOMMON �� F_HIRAGANA �Ƃ��ďo�͂���ꍇ

					case USE_HIRAGANA_BANK_G0_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G0(F_HIRAGANA);				// F_HIRAGANA -> G0
						CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
						WRITEBUF(jisCode);
						break;
					case USE_HIRAGANA_BANK_G1_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G1(F_HIRAGANA);				// F_HIRAGANA -> G1
						CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
						WRITEBUF(jisCode);
						break;
					case  USE_HIRAGANA_BANK_G1_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G1(F_HIRAGANA);				// F_HIRAGANA -> G1
						CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_HIRAGANA_BANK_G2_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G2(F_HIRAGANA);				// F_HIRAGANA -> G2
						CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
						WRITEBUF(jisCode);
						break;
					case USE_HIRAGANA_BANK_G2_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G2(F_HIRAGANA);				// F_HIRAGANA -> G2
						CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_HIRAGANA_BANK_G2_SSHIFT:
						CHECK_AND_SET_1B_GSET_TO_G2(F_HIRAGANA);				// F_HIRAGANA -> G2
						SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
						WRITEBUF(jisCode);
						break;
					case USE_HIRAGANA_BANK_G3_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G3(F_HIRAGANA);				// F_HIRAGANA -> G3
						CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
						WRITEBUF(jisCode);
						break;
					case USE_HIRAGANA_BANK_G3_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G3(F_HIRAGANA);				// F_HIRAGANA -> G3
						CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_HIRAGANA_BANK_G3_SSHIFT:
						CHECK_AND_SET_1B_GSET_TO_G3(F_HIRAGANA);				// F_HIRAGANA -> G3
						SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
						WRITEBUF(jisCode);
						break;

					// F_KANACOMMON �� F_KATAKANA �Ƃ��ďo�͂���ꍇ

					case USE_KATAKANA_BANK_G0_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G0(F_KATAKANA);				// F_KATAKANA -> G0
						CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
						WRITEBUF(jisCode);
						break;
					case USE_KATAKANA_BANK_G1_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G1(F_KATAKANA);				// F_KATAKANA -> G1
						CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
						WRITEBUF(jisCode);
						break;
					case  USE_KATAKANA_BANK_G1_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G1(F_KATAKANA);				// F_KATAKANA -> G1
						CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_KATAKANA_BANK_G2_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G2(F_KATAKANA);				// F_KATAKANA -> G2
						CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
						WRITEBUF(jisCode);
						break;
					case USE_KATAKANA_BANK_G2_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G2(F_KATAKANA);				// F_KATAKANA -> G2
						CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_KATAKANA_BANK_G2_SSHIFT:
						CHECK_AND_SET_1B_GSET_TO_G2(F_KATAKANA);				// F_KATAKANA -> G2
						SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
						WRITEBUF(jisCode);
						break;
					case USE_KATAKANA_BANK_G3_TO_GL:
						CHECK_AND_SET_1B_GSET_TO_G3(F_KATAKANA);				// F_KATAKANA -> G3
						CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
						WRITEBUF(jisCode);
						break;
					case USE_KATAKANA_BANK_G3_TO_GR:
						CHECK_AND_SET_1B_GSET_TO_G3(F_KATAKANA);				// F_KATAKANA -> G3
						CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
						WRITEBUF(jisCode + 0x80);
						break;
					case USE_KATAKANA_BANK_G3_SSHIFT:
						CHECK_AND_SET_1B_GSET_TO_G3(F_KATAKANA);				// F_KATAKANA -> G3
						SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
						WRITEBUF(jisCode);
						break;

					// F_HIARGANA, F_KATAKANA, F_KANACOMMON �� F_JIS1KANJI �Ƃ��ďo�͂���ꍇ�D

					case USE_KANJI_BANK_G0_TO_GL:
						if (cType == F_HIRAGANA) {
							jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
						}
						else {
							jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
						}
						CHECK_AND_SET_2B_GSET_TO_G0(F_JIS1KANJI);							// F_JIS1KANJI -> G0
						CHECK_AND_LSHIFT_G0_IN_GL;											// G0 -> GL (LS0)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					case USE_KANJI_BANK_G1_TO_GL:
						if (cType == F_HIRAGANA) {
							jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
						}
						else {
							jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
						}
						CHECK_AND_SET_2B_GSET_TO_G1(F_JIS1KANJI);							// F_JIS1KANJI -> G1
						CHECK_AND_LSHIFT_G1_IN_GL;											// G1 -> GL (LS1)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					case  USE_KANJI_BANK_G1_TO_GR:
						if (cType == F_HIRAGANA) {
							jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
						}
						else {
							jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
						}
						CHECK_AND_SET_2B_GSET_TO_G1(F_JIS1KANJI);							// F_JIS1KANJI -> G1
						CHECK_AND_LSHIFT_G1_IN_GR;											// G1 -> GR (LS1R)
						WRITEBUF((jisCode >> 8) + 0x80);
						WRITEBUF((jisCode & 0xFF) + 0x80);
						break;
					case USE_KANJI_BANK_G2_TO_GL:
						if (cType == F_HIRAGANA) {
							jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
						}
						else {
							jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
						}
						CHECK_AND_SET_2B_GSET_TO_G2(F_JIS1KANJI);							// F_JIS1KANJI -> G2
						CHECK_AND_LSHIFT_G2_IN_GL;											// G2 -> GL (LS2)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					case USE_KANJI_BANK_G2_TO_GR:
						if (cType == F_HIRAGANA) {
							jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
						}
						else {
							jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
						}
						CHECK_AND_SET_2B_GSET_TO_G2(F_JIS1KANJI);							// F_JIS1KANJI -> G2
						CHECK_AND_LSHIFT_G2_IN_GR;											// G2 -> GR (LS2R)
						WRITEBUF((jisCode >> 8) + 0x80);
						WRITEBUF((jisCode & 0xFF) + 0x80);
						break;
					case USE_KANJI_BANK_G2_SSHIFT:
						if (cType == F_HIRAGANA) {
							jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
						}
						else {
							jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
						}
						CHECK_AND_SET_2B_GSET_TO_G2(F_JIS1KANJI);							// F_JIS1KANJI -> G2
						SSHIFT_G2_IN_GL;													// G2 -> GL (SS2)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					case USE_KANJI_BANK_G3_TO_GL:
						if (cType == F_HIRAGANA) {
							jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
						}
						else {
							jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
						}
						CHECK_AND_SET_2B_GSET_TO_G3(F_JIS1KANJI);							// F_JIS1KANJI -> G3
						CHECK_AND_LSHIFT_G3_IN_GL;											// G3 -> GL (LS3)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;
					case USE_KANJI_BANK_G3_TO_GR:
						if (cType == F_HIRAGANA) {
							jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
						}
						else {
							jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
						}
						CHECK_AND_SET_2B_GSET_TO_G3(F_JIS1KANJI);							// F_JIS1KANJI -> G3
						CHECK_AND_LSHIFT_G3_IN_GR;											// G3 -> GR (LS3R)
						WRITEBUF((jisCode >> 8) + 0x80);
						WRITEBUF((jisCode & 0xFF) + 0x80);
						break;
					case USE_KANJI_BANK_G3_SSHIFT:
						if (cType == F_HIRAGANA) {
							jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
						}
						else {
							jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
						}
						CHECK_AND_SET_2B_GSET_TO_G3(F_JIS1KANJI);							// F_JIS1KANJI -> G3
						SSHIFT_G3_IN_GL;													// G3 -> GL (SS3)
						WRITEBUF(jisCode >> 8);
						WRITEBUF(jisCode & 0xFF);
						break;

					default:
						break;
				}
				break;

			default:
				break;
		}

		src += charLen;
	}

	WRITEBUF(0x00);

	dst--;
	if (dst > maxbufsize) dst = maxbufsize;

	delete[] seq;

	return dst;			// �ϊ���̒�����Ԃ�(uint8_t�P��), �I�[��NULL���������܂܂Ȃ�
}

// -----------------------------------------------------------------------------------------------------------------------------------------------

size_t conv_to_U32T(uint32_t *dbuf, const size_t maxbufsize, const uint8_t *sbuf, const size_t total_length)
{
//		8�P�ʕ��������� -> ���������p32bit������ւ̕ϊ�
//
//		sbuf				�ϊ���buf
//		total_length		���̒���(uint8_t�P��, NULL���������܂܂Ȃ�)
//		dbuf				�ϊ���buf
//		maxbufsize			�ϊ���buf�̍ő�T�C�Y(uint32_t�P��), �z�������͏������܂ꂸ���������
//
//		�߂�l				�ϊ����Đ�������������̒���(uint32_t�P��)
//

	ConvStatus	status;
	initConvStatus(&status);

	size_t		src = 0;
	size_t		dst = 0;

	while (src < total_length)
	{
		if (isControlChar(sbuf[src]))
		{
			// 0x00�`0x20, 0x7F�`0xA0, 0xFF�̏ꍇ

			switch (sbuf[src])
			{
				case 0x08:						// APB (BS)
				case 0x09:						// APF (TAB)
					UINT32TBUF(C_HALF_CONTROL, sbuf[src]);								// BS, TAB�o��
					src++;
					break;
				case 0x0A:						// APD (LF)
				case 0x0D:						// APR (CR)
					UINT32TBUF(C_HALF_CONTROL, 0x000D);									// LF�o��
					if ((sbuf[src] == 0x0D) && ((src + 1) < total_length) && (sbuf[src + 1] == 0x0A)) src++;
					src++;
					break;
				case 0x20:						// SP
					UINT32TBUF(C_HALF_CONTROL, 0x0020 );								// SP�o��
					src++;
					break;
				case 0x7F:						// DEL
					UINT32TBUF(C_HALF_CONTROL, 0x007F);									// DEL�o��
					src++;
					break;
				case 0x9B:						// CSI����
					src += csiProc(sbuf + src, total_length - src, &status);
					break;
				default:						// ����ȊO�̐���R�[�h
					src += changeConvStatus(sbuf + src, total_length - src, &status);
					break;
			}
		}
		else
		{
			// GL, GR�ɑΉ����镶���o��

			int32_t		uc		= 0;
			int32_t		tmpjis	= 0;

			const int32_t	regionLR	= (sbuf[src] >= 0x80) ? REGION_GR : REGION_GL;

			int32_t	jis1	= sbuf[src] & 0x7F;
			int32_t	jis2	= ( src + 1 < total_length ) ? sbuf[src + 1] & 0x7F : 0;
			
			if ((jis1 < 0x21) || (jis1 > 0x7E)) jis1 = 0;
			if ((jis2 < 0x21) || (jis2 > 0x7E)) jis2 = 0;

			const int32_t	jis12 = ((jis1 != 0) && (jis2 != 0)) ? jis1 * 256 + jis2 : 0;

			switch (status.bank[status.region[regionLR]])
			{
				case F_KANJI:
				case F_JIS1KANJI:
					uc = jis12Conv(jis12, true);
					if ((tmpjis = kanaCommon1Conv(uc, false)) != 0) {
						UINT32TBUF(C_HALF_KANACOMMON, tmpjis);
					}
					else if ((tmpjis = hiragana1Conv(uc, false)) != 0) {
						UINT32TBUF(C_HALF_HIRAGANA, tmpjis);
					}
					else if ((tmpjis = katakana1Conv(uc, false)) != 0) {
						UINT32TBUF(C_HALF_KATAKANA, tmpjis);
					}
					else if (jis12 != 0) {
						UINT32TBUF(C_HALF_JIS1KANJI, jis12);
					}
					src += 2;
					break;
				case F_JIS2KANJI:
					if (jis12 != 0) {
						UINT32TBUF(C_HALF_JIS2KANJI, jis12);
					}
					src += 2;
					break;
				case F_ALPHA:
				case F_P_ALPHA:
					if (jis1 != 0) {
						UINT32TBUF(C_HALF_ALPHA, jis1);
					}
					src++;
					break;
				case F_HIRAGANA:
				case F_P_HIRAGANA:
					uc = hiragana1Conv(jis1, true);
					if ((tmpjis = kanaCommon1Conv(uc, false)) != 0) {
						UINT32TBUF(C_HALF_KANACOMMON, tmpjis);
					}
					else if (jis1 != 0) {
						UINT32TBUF(C_HALF_HIRAGANA, jis1);
					}
					src++;
					break;
				case F_KATAKANA:
				case F_P_KATAKANA:
					uc = katakana1Conv(jis1, true);
					if ((tmpjis = kanaCommon1Conv(uc, false)) != 0) {
						UINT32TBUF(C_HALF_KANACOMMON, tmpjis);
					} 
					else if (jis1 != 0) {
						UINT32TBUF(C_HALF_KATAKANA, jis1);
					}
					src++;
					break;
				case F_HANKAKU:
					if ((jis1 > 0x20) && (jis1 < 0x60)) {
						UINT32TBUF(C_HALF_HANKAKU, jis1);
					}
					src++;
					break;
				case F_KIGOU:
					uc = jis12Conv(jis12, true);
					if ((uc != 0) && ((tmpjis = kanaCommon1Conv(uc, false)) != 0)) {
						UINT32TBUF(C_HALF_KANACOMMON, tmpjis);
					}
					else if ((uc != 0) && ((tmpjis = hiragana1Conv(uc, false)) != 0)) {
						UINT32TBUF(C_HALF_HIRAGANA, tmpjis);
					}
					else if ((uc != 0) && ((tmpjis = katakana1Conv(uc, false)) != 0)) {
						UINT32TBUF(C_HALF_KATAKANA, tmpjis);
					}
					else if (uc != 0) {
						UINT32TBUF(C_HALF_JIS1KANJI, jis12);
					}
					else if (jis12 != 0) {
						UINT32TBUF(C_HALF_KIGOU, jis12);
					}
					src += 2;
					break;
				case F_DRCS0:
					UINT32TBUF(C_HALF_JIS1KANJI, 0x2129);						// '�H'�o��
					src += 2;
					break;
				case F_DRCS1A:
				case F_DRCS2A:
				case F_DRCS3A:
				case F_DRCS4A:
				case F_DRCS5A:
				case F_DRCS6A:
				case F_DRCS7A:
				case F_DRCS8A:
				case F_DRCS9A:
				case F_DRCS10A:
				case F_DRCS11A:
				case F_DRCS12A:
				case F_DRCS13A:
				case F_DRCS14A:
				case F_DRCS15A:
					UINT32TBUF(C_HALF_ALPHA, 0x003F);							// '?'�o��
					src++;
					break;
				case F_MACROA:
					defaultMacroProc(sbuf[src] & 0x7F, &status);
					src++;
					break;
				case F_MOSAICA:
				case F_MOSAICB:
				case F_MOSAICC:
				case F_MOSAICD:
				default:
					src++;
					break;
			}

			if (status.bSingleShift) {
				status.region[REGION_GL] = status.region_GL_backup;
				status.bSingleShift = false;
			}
		}

	}

	if (dst < maxbufsize) dbuf[dst] = 0x0000;								// �I�[��NULL����

	if (dst > maxbufsize) dst = maxbufsize; 

	return dst;			// �ϊ���̒�����Ԃ�(uint32_t�P��), �I�[��null���������܂܂Ȃ�
}


size_t conv_from_U32T(uint8_t *dbuf, const size_t maxbufsize, const uint32_t *sbuf, const size_t total_length)
{
	//		���������p32bit������ -> 8�P�ʕ���������ւ̕ϊ�
	//
	//		sbuf				�ϊ���buf
	//		total_length		���̒���(uint32_t�P��, NULL���������܂܂Ȃ�)
	//		dbuf				�ϊ���buf
	//		maxbufsize			�ϊ���buf�̍ő�T�C�Y(uint8_t�P��), �z�������͏������܂ꂸ���������
	//
	//		�߂�l				�ϊ����Đ�������������̒���(uint8_t�P��)
	//
	//		�ȉ���BANK���蓖�ĕ��@�ŕϊ��R�X�g�i�ϊ����ʂ�8�P�ʕ�����̒����j���v�Z���A�R�X�g���ł��������Ȃ���@�ŕϊ����s���D
	//
	//		���蓖�ĕ��@ 
	//			REGION_GL, REGION_GR			: BANKG0, BANKG1, BANKG2, BANKG3 �̉�����g�p����
	//			BANKG0, BANKG1, BANKG2, BANKG3 	: F_JIS1KANJI, F_ALPHA, F_HIRAGANA, F_KATAKANA, F_KIGOU, F_HANKAKU,F_JIS2KANJI �̂��ׂĂ̕�����Ɋ��蓖�Ă�D
	//

	const bool	bCharSize = true;

	ConvStatus	status;
	initConvStatus(&status);

	BankSet		*seq = checkMojiSequenceU32T(sbuf, total_length, &status);

	size_t		cCount = 0;
	uint32_t	cType = status.bank[status.region[REGION_GL]];

	size_t		src = 0;
	size_t		dst = 0;


	// �ϊ����C��

	while (src < total_length)
	{
		int32_t		jisCode;
		uint32_t	charType = classOfCharU32T(sbuf[src], &jisCode);

		uint32_t	charWidth = charType & C_MASK_CHARWIDTH;
		charType = (charType  & C_MASK_CHARCODE) >> 16;

		if ((charType != F_CONTROL) || (jisCode == 0x20))						// �����킪F_CONTROL�ȊO�̎��A�y�ы󔒕����̎��͕����T�C�Y����t�^����B
		{
			if (charWidth == C_FULLSIZE) {
				IF_MSZ_TO_NSZ;
			}
			else {
				IF_NSZ_TO_MSZ;
			}
		}

		switch (charType)
		{
			// ����R�[�h�o�́iSP�܂ށj

		case F_CONTROL:
			switch (jisCode)
			{
			case 0x09:					// TAB
				WRITEBUF(0x09);
				break;
			case 0x0D:
			case 0x0A:					// LF
				WRITEBUF(0x0D);
				break;
			case 0x20:					// SP
				WRITEBUF(0x20);
				break;
			case 0x7F:					// DEL
				WRITEBUF(0x7F);
				break;
			default:
				break;
			}
			break;

			// �����o��

		case F_ALPHA:
		case F_HANKAKU:
			if (charType != cType) {
				cCount++;													// �����킪�؂�ւ����
				cType = charType;
			}

			switch (seq[cCount])											// seq[]�ɏ]���AG0, G1�̂ǂ��炩�AGL, GR�̂ǂ��炩���g��
			{
				case USE_BANK_G0_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G0(charType);					// F -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G1_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
					WRITEBUF(jisCode);
					break;
				case  USE_BANK_G1_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G2_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G2_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G2_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G3_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G3_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G3_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
					WRITEBUF(jisCode);
					break;
				default:
					break;
			}
			break;

		case F_JIS1KANJI:
		case F_JIS2KANJI:
		case F_KIGOU:
			if (charType != cType) {
				cCount++;													// �����킪�؂�ւ����
				cType = charType;
			}

			switch (seq[cCount])											// seq[]�ɏ]���AG0, G1�̂ǂ��炩�AGL, GR�̂ǂ��炩���g��
			{
				case USE_BANK_G0_TO_GL:
					CHECK_AND_SET_2B_GSET_TO_G0(charType);					// F -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_BANK_G1_TO_GL:
					CHECK_AND_SET_2B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case  USE_BANK_G1_TO_GR:
					CHECK_AND_SET_2B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
					break;
				case USE_BANK_G2_TO_GL:
					CHECK_AND_SET_2B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_BANK_G2_TO_GR:
					CHECK_AND_SET_2B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
				case USE_BANK_G2_SSHIFT:
					CHECK_AND_SET_2B_GSET_TO_G2(charType);					// F -> G2
					SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_BANK_G3_TO_GL:
					CHECK_AND_SET_2B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_BANK_G3_TO_GR:
					CHECK_AND_SET_2B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
				case USE_BANK_G3_SSHIFT:
					CHECK_AND_SET_2B_GSET_TO_G3(charType);					// F -> G3
					SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				default:
					break;
			}
			break;

		case F_HIRAGANA:
		case F_KATAKANA:
		case F_KANACOMMON:
			if (charType != cType) {
				cCount++;												// �����킪�؂�ւ����
				cType = charType;
			}

			switch (seq[cCount])										// seq[]�ɏ]���AREGION_GL, REGION_GR�̂ǂ��炩���g��
			{
				// F_HIRAGANA �́A���̂܂܏o�͂���ꍇ�ƁAF_JIS1KANJI �Ƃ��ďo�͂���ꍇ�̓�̑I����������D
				// F_KATAKANA �́A���̂܂܏o�͂���ꍇ�ƁAF_JIS1KANJI �Ƃ��ďo�͂���ꍇ�̓�̑I����������D
				// F_KANACOMMON �́AF_HIRAGANA, F_KATAKANA, F_JIS1KANJI �Ƃ��ďo�͂���ꍇ�̎O�̑I����������D

				// F_HIRAGANA, F_KATAKANA �����̂܂܏o�͂���ꍇ

				case USE_BANK_G0_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G0(charType);					// F -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G1_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
					WRITEBUF(jisCode);
					break;
				case  USE_BANK_G1_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G1(charType);					// F -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G2_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G2_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G2_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G2(charType);					// F -> G2
					SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G3_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
					WRITEBUF(jisCode);
					break;
				case USE_BANK_G3_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_BANK_G3_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G3(charType);					// F -> G3
					SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
					WRITEBUF(jisCode);
					break;

				// F_KANACOMMON �� F_HIRAGANA �Ƃ��ďo�͂���ꍇ

				case USE_HIRAGANA_BANK_G0_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G0(F_HIRAGANA);				// F_HIRAGANA -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
					WRITEBUF(jisCode);
					break;
				case USE_HIRAGANA_BANK_G1_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G1(F_HIRAGANA);				// F_HIRAGANA -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
					WRITEBUF(jisCode);
					break;
				case  USE_HIRAGANA_BANK_G1_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G1(F_HIRAGANA);				// F_HIRAGANA -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_HIRAGANA_BANK_G2_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G2(F_HIRAGANA);				// F_HIRAGANA -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
					WRITEBUF(jisCode);
					break;
				case USE_HIRAGANA_BANK_G2_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G2(F_HIRAGANA);				// F_HIRAGANA -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_HIRAGANA_BANK_G2_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G2(F_HIRAGANA);				// F_HIRAGANA -> G2
					SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
					WRITEBUF(jisCode);
					break;
				case USE_HIRAGANA_BANK_G3_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G3(F_HIRAGANA);				// F_HIRAGANA -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
					WRITEBUF(jisCode);
					break;
				case USE_HIRAGANA_BANK_G3_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G3(F_HIRAGANA);				// F_HIRAGANA -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_HIRAGANA_BANK_G3_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G3(F_HIRAGANA);				// F_HIRAGANA -> G3
					SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
					WRITEBUF(jisCode);
					break;

				// F_KANACOMMON �� F_KATAKANA �Ƃ��ďo�͂���ꍇ

				case USE_KATAKANA_BANK_G0_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G0(F_KATAKANA);				// F_KATAKANA -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;								// G0 -> GL (LS0)
					WRITEBUF(jisCode);
					break;
				case USE_KATAKANA_BANK_G1_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G1(F_KATAKANA);				// F_KATAKANA -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;								// G1 -> GL (LS1)
					WRITEBUF(jisCode);
					break;
				case  USE_KATAKANA_BANK_G1_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G1(F_KATAKANA);				// F_KATAKANA -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;								// G1 -> GR (LS1R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_KATAKANA_BANK_G2_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G2(F_KATAKANA);				// F_KATAKANA -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;								// G2 -> GL (LS2)
					WRITEBUF(jisCode);
					break;
				case USE_KATAKANA_BANK_G2_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G2(F_KATAKANA);				// F_KATAKANA -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;								// G2 -> GR (LS2R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_KATAKANA_BANK_G2_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G2(F_KATAKANA);				// F_KATAKANA -> G2
					SSHIFT_G2_IN_GL;										// G2 -> GL (SS2)
					WRITEBUF(jisCode);
					break;
				case USE_KATAKANA_BANK_G3_TO_GL:
					CHECK_AND_SET_1B_GSET_TO_G3(F_KATAKANA);				// F_KATAKANA -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;								// G3 -> GL (LS3)
					WRITEBUF(jisCode);
					break;
				case USE_KATAKANA_BANK_G3_TO_GR:
					CHECK_AND_SET_1B_GSET_TO_G3(F_KATAKANA);				// F_KATAKANA -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;								// G3 -> GR (LS3R)
					WRITEBUF(jisCode + 0x80);
					break;
				case USE_KATAKANA_BANK_G3_SSHIFT:
					CHECK_AND_SET_1B_GSET_TO_G3(F_KATAKANA);				// F_KATAKANA -> G3
					SSHIFT_G3_IN_GL;										// G3 -> GL (SS3)
					WRITEBUF(jisCode);
					break;

				// F_HIARGANA, F_KATAKANA, F_KANACOMMON �� F_JIS1KANJI �Ƃ��ďo�͂���ꍇ�D
				
				case USE_KANJI_BANK_G0_TO_GL:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G0(F_JIS1KANJI);							// F_JIS1KANJI -> G0
					CHECK_AND_LSHIFT_G0_IN_GL;											// G0 -> GL (LS0)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_KANJI_BANK_G1_TO_GL:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G1(F_JIS1KANJI);							// F_JIS1KANJI -> G1
					CHECK_AND_LSHIFT_G1_IN_GL;											// G1 -> GL (LS1)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case  USE_KANJI_BANK_G1_TO_GR:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G1(F_JIS1KANJI);							// F_JIS1KANJI -> G1
					CHECK_AND_LSHIFT_G1_IN_GR;											// G1 -> GR (LS1R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
				case USE_KANJI_BANK_G2_TO_GL:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G2(F_JIS1KANJI);							// F_JIS1KANJI -> G2
					CHECK_AND_LSHIFT_G2_IN_GL;											// G2 -> GL (LS2)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_KANJI_BANK_G2_TO_GR:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G2(F_JIS1KANJI);							// F_JIS1KANJI -> G2
					CHECK_AND_LSHIFT_G2_IN_GR;											// G2 -> GR (LS2R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
				case USE_KANJI_BANK_G2_SSHIFT:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G2(F_JIS1KANJI);							// F_JIS1KANJI -> G2
					SSHIFT_G2_IN_GL;													// G2 -> GL (SS2)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_KANJI_BANK_G3_TO_GL:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G3(F_JIS1KANJI);							// F_JIS1KANJI -> G3
					CHECK_AND_LSHIFT_G3_IN_GL;											// G3 -> GL (LS3)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;
				case USE_KANJI_BANK_G3_TO_GR:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G3(F_JIS1KANJI);							// F_JIS1KANJI -> G3
					CHECK_AND_LSHIFT_G3_IN_GR;											// G3 -> GR (LS3R)
					WRITEBUF((jisCode >> 8) + 0x80);
					WRITEBUF((jisCode & 0xFF) + 0x80);
					break;
				case USE_KANJI_BANK_G3_SSHIFT:
					if (cType == F_HIRAGANA) {
						jisCode = jis12Conv(hiragana1Conv(jisCode, true), false);
					} else {
						jisCode = jis12Conv(katakana1Conv(jisCode, true), false);
					}
					CHECK_AND_SET_2B_GSET_TO_G3(F_JIS1KANJI);							// F_JIS1KANJI -> G3
					SSHIFT_G3_IN_GL;													// G3 -> GL (SS3)
					WRITEBUF(jisCode >> 8);
					WRITEBUF(jisCode & 0xFF);
					break;

				default:
					break;
			}
			break;

		default:
			break;
		}

		src++;
	}

	WRITEBUF(0x00);

	dst--;
	if (dst > maxbufsize) dst = maxbufsize;

	delete[] seq;

	return dst;			// �ϊ���̒�����Ԃ�(uint8_t�P��), �I�[��NULL���������܂܂Ȃ�
}


size_t convU32T_to_UTF16(char16_t *dbuf, const size_t maxbufsize, const uint32_t *sbuf, const size_t total_length)
{
	//
	// ���������p32bit��������AUTF-16������ɕϊ�����
	// ���̍ہA�����T�C�Y�w��(C_HALF ���邢�� C_FULL)�͖������A�S�p�p�����y�ы󔒕����͔��p�ɕϊ�����
	//

	size_t	dst = 0;

	for (size_t src = 0; src < total_length; src++)
	{
		const uint32_t	charType = sbuf[src] & 0x0FFF0000;
		const int32_t	jis = sbuf[src] & 0x0000FFFF;

		int32_t		uc = 0;
		int32_t		uc2 = 0;
		size_t		len = 0;
		char16_t	utfStr[UTF16TABLELEN] = { 0 };

		switch (charType)
		{
			case C_HALF_CONTROL:
				uc = jis;
				if (uc == 0x000D) {
#ifdef __USE_UTF_CODE_CRLF__
					UTF16BUF2(0x000D);													// CR�o��
#endif
					UTF16BUF2(0x000A);													// LF�o��
				}
				else {
					UTF16BUF2(uc);
				}
				break;
			case C_HALF_ALPHA:
				uc = alphaConv(jis, true);
				if (uc != 0) UTF16BUF2(uc);
				break;
			case C_HALF_KANJI:
			case C_HALF_JIS1KANJI:
				if ((len = jis3CombAndIvsConv(F_JIS1KANJI, jis, &uc, &uc2)) != 0) {
					UTF16BUF2(uc);
					UTF16BUF2(uc2);
					break;
				}
				if (jis == 0x2121) uc = 0x20;										// �S�p�󔒂͔��p�󔒂ɕϊ�����
				if (uc == 0) uc = alphaConv(charSize1Conv(jis, false), true);		// �S�p�p�����͔��p�����ɕϊ�����
				if (uc == 0) uc = jis12Conv(jis, true);
				if (uc == 0) uc = jis3Conv(jis, true);
				if (uc != 0) {
					UTF16BUF2(uc);
				}
				break;
			case C_HALF_JIS2KANJI:
				uc = jis4Conv(jis, true);
				if (uc != 0) UTF16BUF2(uc);
				break;
			case C_HALF_HIRAGANA:
				uc = hiragana1Conv(jis, true);
				if (uc != 0) UTF16BUF2(uc);
				break;
			case C_HALF_KATAKANA:
				uc = katakana1Conv(jis, true);
				if (uc != 0) UTF16BUF2(uc);
				break;
			case C_HALF_KANACOMMON:
				uc = kanaCommon1Conv(jis, true);
				if (uc != 0) UTF16BUF2(uc);
				break;
			case C_HALF_HANKAKU:
				uc = hankaku1Conv(jis, true);
				if (uc != 0) UTF16BUF2(uc);
				break;
			case C_HALF_KIGOU:
				if ((len = jis3CombAndIvsConv(F_KIGOU, jis, &uc, &uc2)) != 0) {
					UTF16BUF2(uc);
					UTF16BUF2(uc2);
					break;
				}
				uc = kigou1Conv(jis, true);
				if (uc != 0) {
					UTF16BUF2(uc);
				}
				else {
					len = kigou2ConvUTF16(jis, utfStr, UTF16TABLELEN);
					dst += writeUTF16StrBuf(dbuf, maxbufsize, dst, utfStr, len, false);
				}
				break;
			default:
				break;
		}
	}

	UTF16BUF2(0x0000);

	dst--;
	if (dst > maxbufsize) dst = maxbufsize;

	return dst;			// �ϊ���̒�����Ԃ�(char16_t�P��), �I�[��null���������܂܂Ȃ�
}


size_t convU32T_to_UTF8(uint8_t *dbuf, const size_t maxbufsize, const uint32_t *sbuf, const size_t total_length)
{
	//
	// ���������p32bit��������AUTF-8������ɕϊ�����
	// ���̍ہA�����T�C�Y�w��(C_HALF ���邢�� C_FULL)�͖������A�S�p�p�����y�ы󔒕����͔��p�ɕϊ�����
	//

	size_t	dst = 0;

	for (size_t src = 0; src < total_length; src++)
	{
		const uint32_t	charType = sbuf[src] & 0x0FFF0000;
		const int32_t	jis = sbuf[src] & 0x0000FFFF;

		int32_t		uc = 0;
		int32_t		uc2 = 0;
		size_t		len = 0;
		uint8_t		utfStr[UTF8TABLELEN] = { 0 };

		switch (charType)
		{
		case C_HALF_CONTROL:
			uc = jis;
			if (uc == 0x0D) {
#ifdef __USE_UTF_CODE_CRLF__
				UTF8BUF2(0x0D);													// CR�o��
#endif
				UTF8BUF2(0x0A);													// LF�o��
			}
			else {
				UTF8BUF2(uc);
			}
			break;
		case C_HALF_ALPHA:
			uc = alphaConv(jis, true);
			if (uc != 0) UTF8BUF2(uc);
			break;
		case C_HALF_KANJI:
		case C_HALF_JIS1KANJI:
			if ((len = jis3CombAndIvsConv(F_JIS1KANJI, jis, &uc, &uc2)) != 0) {
				UTF8BUF2(uc);
				UTF8BUF2(uc2);
				break;
			}
			if (jis == 0x2121) uc = 0x20;										// �S�p�󔒂͔��p�󔒂ɕϊ�����
			if (uc == 0) uc = alphaConv(charSize1Conv(jis, false), true);		// �S�p�p�����͔��p�����ɕϊ�����
			if (uc == 0) uc = jis12Conv(jis, true);
			if (uc == 0) uc = jis3Conv(jis, true);
			if (uc != 0) {
				UTF8BUF2(uc);
			}
			break;
		case C_HALF_JIS2KANJI:
			uc = jis4Conv(jis, true);
			if (uc != 0) UTF8BUF2(uc);
			break;
		case C_HALF_HIRAGANA:
			uc = hiragana1Conv(jis, true);
			if (uc != 0) UTF8BUF2(uc);
			break;
		case C_HALF_KATAKANA:
			uc = katakana1Conv(jis, true);
			if (uc != 0) UTF8BUF2(uc);
			break;
		case C_HALF_KANACOMMON:
			uc = kanaCommon1Conv(jis, true);
			if (uc != 0) UTF8BUF2(uc);
			break;
		case C_HALF_HANKAKU:
			uc = hankaku1Conv(jis, true);
			if (uc != 0) UTF8BUF2(uc);
			break;
		case C_HALF_KIGOU:
			if ((len = jis3CombAndIvsConv(F_KIGOU, jis, &uc, &uc2)) != 0) {
				UTF8BUF2(uc);
				UTF8BUF2(uc2);
				break;
			}
			uc = kigou1Conv(jis, true);
			if (uc != 0) {
				UTF8BUF2(uc);
			}
			else {
				len = kigou2ConvUTF8(jis, utfStr, UTF8TABLELEN);
				dst += writeUTF8StrBuf(dbuf, maxbufsize, dst, utfStr, len, false);
			}
			break;
		default:
			break;
		}
	}

	UTF8BUF2(0x0000);

	dst--;
	if (dst > maxbufsize) dst = maxbufsize;

	return dst;			// �ϊ���̒�����Ԃ�(uint8_t�P��), �I�[��null���������܂܂Ȃ�
}

// -----------------------------------------------------------------------------------------------------------------------------------------------

int32_t alphaConv(const int32_t code, const bool bConvDir)
{
//	�p���W�������ɂ��Ă� jiscode <-> unicode�ϊ�
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		����jiscode -> �߂�lunicode
//	bConvDir : false	����unicode -> �߂�ljiscode

	if( (code == 0x5C) && bConvDir ) return 0x00A5;				// jis 0x5C -> yen sign	
	if( (code == 0x7E) && bConvDir ) return 0x203E;				// jis 0x7E -> overline

	if( (code == 0x00A5) && !bConvDir ) return 0x5C;			// yen sign -> jis 0x5C
	if( (code == 0x203E) && !bConvDir ) return 0x7E;			// overline -> jis 0x7E

	if( (code >= 0x21) && (code <= 0x7E) ) return code;			// ��̕����ȊO��jiscode��unicode������Ƃ���

	return 0;
}


int32_t jis12Conv(const int32_t code, const bool bConvDir)
{
//	JIS����񐅏������C�񊿎��ɂ��Ă� jiscode <-> unicode�ϊ�
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		����jiscode -> �߂�lunicode (�� 0x2121 -> U+3000)
//	bConvDir : false	����unicode -> �߂�ljiscode

	static bool			bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(jis12Table, jis12RevTable, JIS12TABLESIZE);
		bTableInitialized = true;
	}

#ifdef _WIN32
	int32_t		winResult = jis12WinConv(code, bConvDir);			// WINDOWS�ŗL�̃}�b�s���O��L������̂ɂ��Ă̕ϊ�
	if(winResult != 0) return winResult;
#else
	if(!bConvDir) {													// ��WINDOWS�ł����Ă��Aunicode->JIS�ϊ��ł� WINDOWS�ŗL�̃}�b�s���O����荞��
		int32_t		winResult = jis12WinConv(code, false);
		if(winResult != 0) return winResult;
	}
#endif

	void	*result;

	if(bConvDir) {
		result = bsearch(&code, jis12Table, JIS12TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&code, jis12RevTable, JIS12TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result == NULL) return 0;

	return *((int32_t*)result + 1);
}


int32_t jis12WinConv(const int32_t code, const bool bConvDir)
{
//	JIS����񐅏��񊿎���windows�ŗL�̃}�b�s���O��L������̂ɂ��Ă� jiscode <-> unicode�ϊ�
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		����jiscode -> �߂�lunicode
//	bConvDir : false	����unicode -> �߂�ljiscode

	static bool			bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(jis12WinTable, jis12WinRevTable, JIS12WINTABLESIZE);
		bTableInitialized = true;
	}
	
	void	*result;

	if(bConvDir) {
		result = bsearch(&code, jis12WinTable, JIS12WINTABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&code, jis12WinRevTable, JIS12WINTABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result == NULL) return 0;

	return *((int32_t*)result + 1);
}


int32_t jis3Conv(const int32_t code, const bool bConvDir)
{
//	JIS��O���������񊿎����Ă� jiscode <-> unicode�ϊ�
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		����jiscode -> �߂�lunicode
//	bConvDir : false	����unicode -> �߂�ljiscode

	static bool			bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(jis3Table, jis3RevTable, JIS3TABLESIZE);
		bTableInitialized = true;
	}
	
	void	*result;

	if(bConvDir) {
		result = bsearch(&code, jis3Table, JIS3TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&code, jis3RevTable, JIS3TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result == NULL) return 0;

	return *((int32_t*)result + 1);
}


size_t jis3CombAndIvsConv(const int32_t cType, const int32_t jis, int32_t *uc1, int32_t *uc2)
{
	//	JIS��O���������񊿎���unicode���������ɑΉ�������̂ɂ��Ă� jiscode -> unicode ������ϊ�
	//�@�����
	//�@JIS��1,2��������, �ǉ������̂����A�ّ̎��Z���N�^�[�t�ň������̂ɂ��Ă� jiscode -> unicode ������ϊ�
	//	�Ή����镶���R�[�h���Ȃ���Ζ߂�l0��Ԃ�
	//
	//	cType		�ϊ���������iF_JIS1KANJI, F_KIGOU)
	//	jis			�ϊ���jiscode
	//	uc1			�ϊ�����unicode�̑�1����
	//	uc2			�ϊ�����unicode�̑�2����
	//	�߂�l		�ϊ�����unicode������

	static bool			bTableInitialized = false;

	if (!bTableInitialized) {
		qsort(jis3CombIvsTable, JIS3COMBIVSTABLESIZE / sizeof(int32_t) / 4, sizeof(int32_t) * 4, compareForTable64);
		bTableInitialized = true;
	}

	int64_t		code	= ((int64_t)jis << 32) + cType;
	void		*result	= bsearch(&code, jis3CombIvsTable, JIS3COMBIVSTABLESIZE / sizeof(int32_t) / 4, sizeof(int32_t) * 4, compareForTable64);

	if (result == NULL) return 0;

	*uc1 = *((int32_t*)result + 2);
	*uc2 = *((int32_t*)result + 3);

	return 2;
}


size_t jis3CombAndIvsRevConv(const int32_t uc1, const int32_t uc2, int32_t *cType, int32_t *jis)
{
	//	JIS��O���������񊿎���unicode���������ɑΉ�������̂ɂ��Ă� unicode -> jiscode ������ϊ�
	//�@�����
	//�@JIS��1,2��������, �ǉ������̂����A�ّ̎��Z���N�^�[�t�ň������̂ɂ��Ă� unicode -> jiscode ������ϊ�
	//	�Ή����镶���R�[�h���Ȃ���Ζ߂�l0��Ԃ�
	//
	//	uc1			�ϊ���unicode�̑�1����
	//	uc2			�ϊ���unicode�̑�2����
	//	cType		�ϊ�����������iF_JIS1KANJI, F_KIGOU)
	//	jis			�ϊ�����jiscode
	//	�߂�l		�ϊ�����unicode������

	static bool			bTableInitialized = false;

	if (!bTableInitialized) {
		memcpy(jis3CombIvsRevTable, jis3CombIvsTable, JIS3COMBIVSTABLESIZE);
		qsort(jis3CombIvsRevTable, JIS3COMBIVSTABLESIZE / sizeof(int32_t) / 4, sizeof(int32_t) * 4, compareForTable64Rev);
		bTableInitialized = true;
	}

	int64_t		code[2];
	code[1]	= ((int64_t)uc2 << 32) + uc1;

	void	*result	= bsearch(code, jis3CombIvsRevTable, JIS3COMBIVSTABLESIZE / sizeof(int32_t) / 4, sizeof(int32_t) * 4, compareForTable64Rev);

	if (result == NULL) return 0;

	*cType	= *((int32_t*)result);
	*jis	= *((int32_t*)result + 1);

	return 2;
}


int32_t jis4Conv(const int32_t code, const bool bConvDir)
{
//	JIS��l���������ɂ��Ă� jiscode <-> unicode�ϊ�
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		����jiscode -> �߂�lunicode
//	bConvDir : false	����unicode -> �߂�ljiscode

	static bool			bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(jis4Table, jis4RevTable, JIS4TABLESIZE);
		bTableInitialized = true;
	}
	
	void	*result;

	if(bConvDir) {
		result = bsearch(&code, jis4Table, JIS4TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&code, jis4RevTable, JIS4TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result == NULL) return 0;

	return *((int32_t*)result + 1);
}


int32_t hiragana1Conv(const int32_t code, const bool bConvDir)
{
//	�S�p�Ђ炪�ȕ����ɂ��Ă� jiscode <-> unicode�ϊ�
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		����jiscode -> �߂�lunicode
//	bConvDir : false	����unicode -> �߂�ljiscode

	static bool			bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(hiragana1Table, hiragana1RevTable, HIRAGANA1TABLESIZE);
		bTableInitialized = true;
	}

	void	*result;

	if(bConvDir) {
		result = bsearch(&code, hiragana1Table, HIRAGANA1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&code, hiragana1RevTable, HIRAGANA1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result == NULL) return 0;

	return *((int32_t*)result + 1);
}


int32_t katakana1Conv(const int32_t code, const bool bConvDir)
{
//	�S�p�J�^�J�i�����ɂ��Ă� jiscode <-> unicode�ϊ�
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		����jiscode -> �߂�lunicode
//	bConvDir : false	����unicode -> �߂�ljiscode

	static bool			bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(katakana1Table, katakana1RevTable, KATAKANA1TABLESIZE);
		bTableInitialized = true;
	}

	void	*result;

	if(bConvDir) {
		result = bsearch(&code, katakana1Table, KATAKANA1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&code, katakana1RevTable, KATAKANA1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result == NULL) return 0;

	return *((int32_t*)result + 1);
}


int32_t kanaCommon1Conv(const int32_t code, const bool bConvDir)
{
//	�S�p�Ђ炪�ȁC�J�^�J�i�W���̋��ʕ����ɂ��Ă� jiscode <-> unicode�ϊ�
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		����jiscode -> �߂�lunicode
//	bConvDir : false	����unicode -> �߂�ljiscode

	static bool			bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(kanaCommon1Table, kanaCommon1RevTable, KANACOMMON1TABLESIZE);
		bTableInitialized = true;
	}

	void	*result;

	if(bConvDir) {
		result = bsearch(&code, kanaCommon1Table, KANACOMMON1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&code, kanaCommon1RevTable, KANACOMMON1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result == NULL) return 0;

	return *((int32_t*)result + 1);
}


int32_t hankaku1Conv(const int32_t code, const bool bConvDir)
{
//	JIS X0201�J�^�J�i�����W���ɂ��Ă� jiscode <-> unicode�ϊ�
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		����jiscode -> �߂�lunicode
//	bConvDir : false	����unicode -> �߂�ljiscode

	static bool			bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(hankaku1Table, hankaku1RevTable, HANKAKU1TABLESIZE);
		bTableInitialized = true;
	}

	void	*result;

	if(bConvDir) {
		result = bsearch(&code, hankaku1Table, HANKAKU1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&code, hankaku1RevTable, HANKAKU1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result == NULL) return 0;

	return *((int32_t*)result + 1);
}


int32_t kigou1Conv(const int32_t code, const bool bConvDir)
{
//	�ǉ��L���C�ǉ������W���̂����A�Ή�����unicode���������݂��镶���ɂ��Ă� jiscode <-> unicode�ϊ�
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		����jiscode -> �߂�lunicode
//	bConvDir : false	����unicode -> �߂�ljiscode

	static bool		bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(kigou1Table, kigou1RevTable, KIGOU1TABLESIZE);
		bTableInitialized = true;
	}

	void	*result;

	if(bConvDir) {
		result = bsearch(&code, kigou1Table, KIGOU1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&code, kigou1RevTable, KIGOU1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result == NULL) return 0;

	int32_t		resultCode = *((int32_t*)result + 1);

	return resultCode;
}


size_t kigou2ConvUTF16(const int32_t jis, char16_t *dbuf, const size_t bufsize)
{
//	�ǉ��L���W���̂����A�Ή�����unicode���������݂��镶�������݂���
//	[...]���邢��[#xx#xx]�ŕ\����镶���ɂ��Ă� jiscode -> UTF-16������ϊ�
//
//	jis			�ϊ���jiscode
//	dbuf		�ϊ����������񂪏������܂��buf
//  bufsize		�ϊ���dbuf�̍ő�T�C�Y(char16_t�P��)
//	�߂�l		�ϊ�����UTF-16������(char16_t�P��)
    
	size_t		len		= 0;
	char16_t	jisbuf	= (char16_t)jis;
    
	void	*result = bsearch(&jisbuf, kigou2TableUTF16, KIGOU2TABLEUTF16SIZE / sizeof(char16_t) / UTF16TABLELEN / 2, sizeof(char16_t) * UTF16TABLELEN * 2, compareForTable2UTF16);
 
	if(result != NULL)
	{
		char16_t	*sbuf = (char16_t*)result + UTF16TABLELEN;

		while( (len < bufsize) && (sbuf[len] != 0x0000) ) {
			dbuf[len] = sbuf[len];
			len++;
		}
	}
	else if (((jis / 256) > 0x20) && ((jis / 256) < 0x7F) && ((jis % 256) > 0x20) && ((jis % 256) < 0x7F))
	{
		uint8_t		sbuf[UTF8TABLELEN];
		_SNPRINTF((char*)sbuf, UTF8TABLELEN, "[#%.2d#%.2d]", (jis / 256) - 32, (jis % 256) - 32);

		while( (len < bufsize) && (sbuf[len] != 0x00) ) {
			dbuf[len] = (char16_t)sbuf[len];
			len++;
		}
	}

	return len;
}


size_t kigou2ConvUTF8(const int32_t jis, uint8_t *dbuf, const size_t bufsize)
{
//	�ǉ��L���W���̂����A�Ή�����unicode���������݂��镶�������݂���
//	[...]���邢��[#xx#xx]�ŕ\����镶���ɂ��Ă� jiscode -> UTF-8������ϊ�
//
//	jis			�ϊ���jiscode
//	dbuf		�ϊ����������񂪏������܂��buf
//  bufsize		�ϊ���dbuf�̍ő�T�C�Y(uint8_t�P��)
//	�߂�l		�ϊ�����UTF-8������(uint8_t�P��)

	size_t			len = 0;
	uint8_t			jisbuf[2];
    
	jisbuf[0]	= (jis & 0xFF00) >> 8;
	jisbuf[1]	= jis & 0x00FF;
    
	void	*result = bsearch(jisbuf, kigou2TableUTF8, KIGOU2TABLEUTF8SIZE / sizeof(uint8_t) / UTF8TABLELEN / 2, sizeof(uint8_t) * UTF8TABLELEN * 2, compareForTable2UTF8);

	if(result != NULL)
	{
		uint8_t	*sbuf = (uint8_t*)result + UTF8TABLELEN;

		while( (len < bufsize) && (sbuf[len] != 0x00) ) {
			dbuf[len] = sbuf[len];
			len++;
		}
	}
	else if (((jis / 256) > 0x20) && ((jis / 256) < 0x7F) && ((jis % 256) > 0x20) && ((jis % 256) < 0x7F))
	{
		uint8_t		sbuf[UTF8TABLELEN];
		_SNPRINTF((char*)sbuf, UTF8TABLELEN, "[#%.2d#%.2d]", (jis / 256) - 32, (jis % 256) - 32);

		while( (len < bufsize) && (sbuf[len] != 0x00) ) {
			dbuf[len] = sbuf[len];
			len++;
		}
	}

	return len;

}


size_t kigou2RevConvUTF16(const char16_t* sbuf, const size_t slen, int32_t *jis)
{
//	�ǉ��L���W���̂����A�Ή�����unicode���������݂���
//	[...]���邢��[#xx#xx]�ŕ\����镶���ɂ��Ă� UTF-16������ -> jiscode�ϊ�
//
//	sbuf		�ϊ���buf
//	slen		�ϊ���buf�̒���(char16_t�P��)
//  jis			�ϊ����ꂽjis�R�[�h������   
//	�߂�l		�ϊ�����UTF-16������(char16_t�P��)
    
	static bool		bTableInitialized = false;
    
	if(!bTableInitialized)
    {
		memcpy(kigou2RevTableUTF16, kigou2TableUTF16, KIGOU2TABLEUTF16SIZE);
        qsort(kigou2RevTableUTF16, KIGOU2TABLEUTF16SIZE / sizeof(char16_t) / UTF16TABLELEN / 2, sizeof(char16_t) * UTF16TABLELEN * 2, compareForTable2StrUTF16);
		bTableInitialized = true;
	}
    
	char16_t	cmpbuf[UTF16TABLELEN];	

	if(slen < UTF16TABLELEN) {													// ��r�̕֋X�̂��߂Ƀo�b�t�@�ɃR�s�[����isbuf�͈̔͊O��ǂݏo���Ă��܂��̂�����邽�߁j
		memcpy(cmpbuf, sbuf, sizeof(char16_t) * slen);
		cmpbuf[slen] = 0x0000;
	}
	
	const char16_t	*srcbuf = (slen < UTF16TABLELEN) ? cmpbuf : sbuf;

	void	*result = bsearch(srcbuf - UTF16TABLELEN, kigou2RevTableUTF16, KIGOU2TABLEUTF16SIZE / sizeof(char16_t) / UTF16TABLELEN / 2, sizeof(char16_t) * UTF16TABLELEN * 2, compareForTable2StrUTF16);

	if(result != NULL)
    {
		size_t	len = cmpStrUTF16(srcbuf, (char16_t*)result + UTF16TABLELEN);

		if(len != 0) {															// ������̕�����v�̉\��������
			*jis = (int32_t)(*(char16_t*)result);
			return len;
		} 
	}
 
	if(	(srcbuf[0] == '[') && (srcbuf[1] == '#') && isdigit(srcbuf[2]) && isdigit(srcbuf[3]) && (srcbuf[4] == '#') && isdigit(srcbuf[5]) && isdigit(srcbuf[6]) && (srcbuf[7] == ']') )
	{
		*jis = ((srcbuf[2] - '0') * 10 + (srcbuf[3] - '0')) * 256 + (srcbuf[5] - '0') * 10 + (srcbuf[6] - '0') + 0x2020;
		return 8;
	} 

	*jis = 0;
    
    return 0;
}


size_t kigou2RevConvUTF8(const uint8_t* sbuf, const size_t slen, int32_t *jis)
{
//	�ǉ��L���W���̂����A�Ή�����unicode���������݂���
//	[...]���邢��[#xx#xx]�ŕ\����镶���ɂ��Ă� UTF-16������ -> jiscode�ϊ�
//
//	sbuf		�ϊ���buf
//	slen		�ϊ���buf�̒���(uint8_t�P��)
//  jis			�ϊ����ꂽjis�R�[�h������   
//	�߂�l		�ϊ�����UTF-8������(uint8_t�P��)

	static bool		bTableInitialized = false;
    
	if(!bTableInitialized)
    {
		memcpy(kigou2RevTableUTF8, kigou2TableUTF8, KIGOU2TABLEUTF8SIZE);
        qsort(kigou2RevTableUTF8, KIGOU2TABLEUTF8SIZE / sizeof(uint8_t) / UTF8TABLELEN / 2, sizeof(uint8_t) * UTF8TABLELEN * 2, compareForTable2StrUTF8);
		bTableInitialized = true;
	}

	uint8_t	cmpbuf[UTF8TABLELEN];	

	if(slen < UTF8TABLELEN) {													// ��r�̕֋X�̂��߂Ƀo�b�t�@�ɃR�s�[����isbuf�͈̔͊O��ǂݏo���Ă��܂��̂�����邽�߁j
		memcpy(cmpbuf, sbuf, sizeof(uint8_t) * slen);
		cmpbuf[slen] = 0x0000;
	}
	
	const uint8_t	*srcbuf = (slen < UTF8TABLELEN) ? cmpbuf : sbuf;

	void	*result = bsearch(srcbuf - UTF8TABLELEN, kigou2RevTableUTF8, KIGOU2TABLEUTF8SIZE / sizeof(uint8_t) / UTF8TABLELEN / 2, sizeof(uint8_t) * UTF8TABLELEN * 2, compareForTable2StrUTF8);

	if(result != NULL)
    {
		size_t	len = cmpStrUTF8(srcbuf, (uint8_t*)result + UTF8TABLELEN);

		if(len != 0) {															// ������̕�����v�̉\��������
			*jis = *((uint8_t*)result) * 256 + *((uint8_t*)result + 1);
			return len;
		} 
	}

    if( (srcbuf[0]=='[') && (srcbuf[1]=='#') && isdigit(srcbuf[2]) && isdigit(srcbuf[3]) && (srcbuf[4]=='#') && isdigit(srcbuf[5]) && isdigit(srcbuf[6]) && (srcbuf[7]==']') )
    {
        *jis = ((srcbuf[2] - '0') * 10 + (srcbuf[3] - '0')) * 256 + (srcbuf[5] - '0') * 10 + (srcbuf[6] - '0') + 0x2020;
        return 8;
    } 

    *jis = 0;

	return 0;
}


int32_t charSize1Conv(const int32_t jis, const bool bConvDir)
{
//�@�󔒕����y�щp�������� ���pjiscode <-> �S�pjiscode �ϊ�
//�@��FA(0x41) <-> �`(0x2341)
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		�������pjiscode -> �߂�l�S�pjiscode
//	bConvDir : false	�����S�pjiscode -> �߂�l���pjiscode

	static bool			bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(charSize1Table, charSize1RevTable, CHARSIZE1TABLESIZE);
		bTableInitialized = true;
	}

	void	*result;

	if(bConvDir) {
		result = bsearch(&jis, charSize1Table, CHARSIZE1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&jis, charSize1RevTable, CHARSIZE1TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result != NULL) return *((int32_t*)result + 1);

	return 0;
}


int32_t charSize2Conv(const int32_t jis, const bool bConvDir)
{
//�@JISX0201�Љ����i�����锼�p�J�i�j�Ɋւ���A���pjiscode <-> �S�pjiscode �ϊ�
//�@��F�(0x31) <-> �A(0x2522)
//	�Ή����镶���R�[�h���Ȃ����0��Ԃ�
//
//	bConvDir : true		�������pjiscode -> �߂�l�S�pjiscode
//	bConvDir : false	�����S�pjiscode -> �߂�l���pjiscode

	static bool			bTableInitialized = false;

	if(!bConvDir && !bTableInitialized) {
		initRevTable(charSize2Table, charSize2RevTable, CHARSIZE2TABLESIZE);
		bTableInitialized = true;
	}

	void	*result;

	if(bConvDir) {
		result = bsearch(&jis, charSize2Table, CHARSIZE2TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	} else {
		result = bsearch(&jis, charSize2RevTable, CHARSIZE2TABLESIZE / sizeof(int32_t) / 2, sizeof(int32_t) * 2, compareForTable);
	}

	if(result != NULL) return *((int32_t*)result + 1);

	return 0;
}


void initRevTable(const int32_t *tabletop, int32_t *revtop, const size_t tablesize)		// �ϊ��e�[�u���������p
{
	const size_t	num = tablesize / sizeof(int32_t) / 2;
		
	for(size_t i = 0; i < num; i++) {
		revtop[i * 2]		= tabletop[i * 2 + 1];
		revtop[i * 2 + 1]	= tabletop[i * 2];
	}

	qsort(revtop, num, sizeof(int32_t) * 2, compareForTable);
	
	return;
}


int compareForTable(const void *item1, const void *item2)						// �ϊ��e�[�u���\�[�g�C�����p�֐�
{
	return *(int32_t*)item1 - *(int32_t*)item2;
}


int compareForTable64(const void *item1, const void *item2)						// �ϊ��e�[�u���\�[�g�C�����p�֐�
{
	int64_t	diff = *(int64_t*)item1 - *(int64_t*)item2;

	if (diff > 0) {
		return 1;
	}
	else if (diff == 0) {
		return 0;
	}

	return -1;
}


int compareForTable64Rev(const void *item1, const void *item2)					// �ϊ��e�[�u���\�[�g�C�����p�֐�
{
	int64_t diff = *((int64_t*)item1 + 1) - *((int64_t*)item2 + 1);

	if (diff > 0) {
		return 1;
	}
	else if (diff == 0) {
		return 0;
	}
	
	return -1;
}


int compareForTable2UTF16(const void *item1, const void *item2)					// �ϊ��e�[�u���\�[�g�C�����p�֐�
{
	return (int)(*(char16_t*)item1) - (int)(*(char16_t*)item2);
}


int compareForTable2UTF8(const void *item1, const void *item2)					// �ϊ��e�[�u���\�[�g�C�����p�֐�
{
	return *(uint8_t*)item1 * 256 + *((uint8_t*)item1 + 1) - *(uint8_t*)item2 * 256 - *((uint8_t*)item2 + 1);
}


int compareForTable2StrUTF16(const void *item1, const void *item2)				// �ϊ��e�[�u���\�[�g�C�����p�֐�
{ 
    char16_t *str1 = (char16_t*)item1 + UTF16TABLELEN;
    char16_t *str2 = (char16_t*)item2 + UTF16TABLELEN;
    
    int32_t		i = 0;
    
	while( (str1[i] != 0) && (str2[i] != 0) ) {
		if(str1[i] != str2[i]) return (int)(str1[i]) - (int)(str2[i]);
		i++;
	}
    
	return 0;
}


int compareForTable2StrUTF8(const void *item1, const void *item2)				// �ϊ��e�[�u���\�[�g�C�����p�֐�
{ 
    uint8_t *str1 = (uint8_t*)item1 + UTF8TABLELEN;
    uint8_t *str2 = (uint8_t*)item2 + UTF8TABLELEN;
    
    int32_t		i = 0;
    
	while( (str1[i] != 0) && (str2[i] != 0) ) {
		if(str1[i] != str2[i]) return (int)(str1[i]) - (int)(str2[i]);
		i++;
	}
    
	return 0;
}


size_t cmpStrUTF16(const char16_t* src, const char16_t* dst)
{
// ������src��������dst�ƈ�v���Ă��邩�`�F�b�N���A��v���Ă���Ȃ炻�̒�����Ԃ�(char16_t�P��)
// dst����0x0000�ŏI�[���镶����
// src����0x0000�ŏI�[���Ă���K�v�͂Ȃ�

	size_t		len	= 0;

	while(len < UTF16TABLELEN)
	{
		if(dst[len] == 0x0000) return len;

		if(src[len] != dst[len]) break;

		len++;
	}

	return 0;
}


size_t cmpStrUTF8(const uint8_t* src, const uint8_t* dst)
{
// ������src��������dst�ƈ�v���Ă��邩�`�F�b�N���A��v���Ă���Ȃ炻�̒�����Ԃ�(uint8_t�P��)
// dst����0x0000�ŏI�[���镶����
// src����0x0000�ŏI�[���Ă���K�v�͂Ȃ�

	size_t		len	= 0;

	while(len < UTF8TABLELEN)
	{
		if(dst[len] == 0x00) return len;

		if(src[len] != dst[len]) break;

		len++;
	}

	return 0;
}


void initConvStatus(ConvStatus *status)
{
	status->region[REGION_GL]	= BANK_G0;
	status->region[REGION_GR]	= BANK_G2;
	
	status->bank[BANK_G0]		= F_JIS1KANJI;
	status->bank[BANK_G1]		= F_ALPHA;
	status->bank[BANK_G2]		= F_HIRAGANA;
	status->bank[BANK_G3]		= F_KATAKANA;

	status->bSingleShift		= false;
	status->region_GL_backup	= BANK_G0;		// backup for singleshift

	status->bXCS				= false;
	status->bNormalSize			= true;			// true: NSZ, false: MSZ

	return;
}


bool isControlChar(const uint8_t c)
{
	if( (c >= 0x00) && (c <= 0x20) ) return true;

	if( (c >= 0x7f) && (c <= 0xa0) ) return true;

	if( c == 0xff ) return true;

	return false;
}


bool isUcControlChar(const int32_t uc)
{
	if( (uc >= 0x00) && (uc <= 0x20) ) return true;

	if( (uc >= 0x7f) && (uc <= 0xa0) ) return true;

	return false;
}


bool isOneByteGSET(const uint8_t c)
{
	bool	bResult;

	switch(c)
	{
		case F_ALPHA:
		case F_HIRAGANA:
		case F_KATAKANA:
		case F_MOSAICA:
		case F_MOSAICB:
		case F_MOSAICC:
		case F_MOSAICD:
		case F_P_ALPHA:
		case F_P_HIRAGANA:
		case F_P_KATAKANA:
		case F_HANKAKU:
			bResult = true;
			break;
		default:
			bResult = false;
	}

	return bResult;
}


bool isTwoByteGSET(const uint8_t c)
{
	bool	bResult;

	switch(c)
	{
		case F_KANJI:
		case F_JIS1KANJI:
		case F_JIS2KANJI:
		case F_KIGOU:
			bResult = true;
			break;
		default:
			bResult = false;
	}

	return bResult;
}


bool isOneByteDRCS(const uint8_t c)
{
	bool	bResult;

	switch(c)
	{
		case F_DRCS1:
		case F_DRCS2:
		case F_DRCS3:
		case F_DRCS4:
		case F_DRCS5:
		case F_DRCS6:
		case F_DRCS7:
		case F_DRCS8:
		case F_DRCS9:
		case F_DRCS10:
		case F_DRCS11:
		case F_DRCS12:
		case F_DRCS13:
		case F_DRCS14:
		case F_DRCS15:
		case F_MACRO:
			bResult = true;
			break;
		default:
			bResult = false;
	}

	return bResult;
}


bool isTwoByteDRCS(const uint8_t c)
{
	bool	bResult;

	switch(c)
	{
		case F_DRCS0:
			bResult = true;
			break;
		default:
			bResult = false;
	}

	return bResult;
}


int32_t numGBank(const uint8_t c)
{
	int32_t		numBank;

	switch(c)
	{
		case 0x28:
			numBank = BANK_G0;
			break;
		case 0x29:
			numBank = BANK_G1;
			break;
		case 0x2A:
			numBank = BANK_G2;
			break;
		case 0x2B:
			numBank = BANK_G3;
			break;
		default:
			numBank = BANK_G0;
	}

	return numBank;
}


size_t writeUTF16Buf(char16_t *dbuf, const size_t maxbufsize, const size_t dst, const int32_t uc, const bool bXCS)
{
	if(bXCS) return 0;

	if(uc >= 0x10000)
	{
		if( (dbuf != NULL) && (dst + 0 < maxbufsize) ) dbuf[dst + 0] = (char16_t)(0xD800 + ((uc - 0x10000) / 0x400));
		if( (dbuf != NULL) && (dst + 1 < maxbufsize) ) dbuf[dst + 1] = (char16_t)(0xDC00 + ((uc - 0x10000) % 0x400));

		return 2;
	}

	if( (dbuf != NULL) && (dst < maxbufsize) ) dbuf[dst] = (char16_t)uc;

	return 1;
}


size_t writeUTF8Buf(uint8_t *dbuf, const size_t maxbufsize, const size_t dst, const int32_t uc, const bool bXCS)
{
	if(bXCS) return 0;

	int32_t		uclen;
	uint8_t		ucbuf[4];

	if(uc < 0) {
		return 0;
	}
	else if (uc <= 0x7F) {
		uclen = 1;
		ucbuf[0] = uc;
	}
	else if (uc <= 0x7FF) {
		uclen = 2;
		ucbuf[0] = 0xC0 | (uc >> 6);
		ucbuf[1] = 0x80 | (uc & 0x3F);
	}
	else if (uc <= 0xFFFF) {
		uclen = 3;
		ucbuf[0] = 0xE0 | (uc >> 12);
		ucbuf[1] = 0x80 | ( (uc >> 6) & 0x3F );
		ucbuf[2] = 0x80 | (uc & 0x3F);
	}
	else if (uc <= 0x10FFFF) {
		uclen = 4;
		ucbuf[0] = 0xF0 | (uc >> 18);
		ucbuf[1] = 0x80 | ( (uc >> 12) & 0x3F );
		ucbuf[2] = 0x80 | ( (uc >> 6) & 0x3F );
		ucbuf[3] = 0x80 | (uc & 0x3F);
	} else {
		return 0;
	}

	if(dbuf != NULL)
	{
		for(int32_t i = 0; i < uclen; i++) {
			if( (dst + i) < maxbufsize ) dbuf[dst + i] = ucbuf[i];
		}
	}

	return (size_t)uclen;
}


size_t writeU32TBuf(uint32_t *dbuf, const size_t maxbufsize, const size_t dst, const uint32_t c_code, const uint32_t code, const ConvStatus *status)
{
	if (status->bXCS) return 0;

	if (dst < maxbufsize)
	{
		uint32_t cSizeInfo = C_FULLSIZE;

		if ((code == 0x08) || (code == 0x09) || (code == 0x0D) || (code == 0x7f) || !status->bNormalSize) cSizeInfo = 0;

		dbuf[dst] =  c_code + code + cSizeInfo;
	}

	return 1;
}


size_t writeUTF16StrBuf(char16_t *dbuf, const size_t maxbufsize, const size_t dst, char16_t *sbuf, const size_t slen, const bool bXCS)
{
	if(bXCS) return 0;

	if(dbuf != NULL)
	{
		for(size_t i = 0; i < slen; i++)    {
			if(dst + i < maxbufsize) dbuf[dst + i] = sbuf[i];
		}
	}

	return slen;
}


size_t writeUTF8StrBuf(uint8_t *dbuf, const size_t maxbufsize, const size_t dst, uint8_t *sbuf, const size_t slen, const bool bXCS)
{
	if(bXCS) return 0;

	if(dbuf != NULL)
	{
		for(size_t i = 0; i < slen; i++)    {
			if(dst + i < maxbufsize) dbuf[dst + i] = sbuf[i];
		}
	}

	return slen;
}


void writeBuf(uint8_t *dbuf, const size_t maxbufsize, const size_t dst, const uint8_t code)
{
	if( (dbuf != NULL) && (dst < maxbufsize) ) dbuf[dst] = code;

	return;
}


size_t changeConvStatus(const uint8_t *srcbuf, size_t slen, ConvStatus *status)
{
// ����R�[�h�ɏ]���ĕ����̌Ăяo���A�w�����䂷��
// 

	uint8_t		tmpbuf[MAXCONTROLSEQUENCE];	

	if(slen < MAXCONTROLSEQUENCE) {													// ��r�̕֋X�̂��߂Ƀo�b�t�@�ɃR�s�[����isbuf�͈̔͊O��ǂݏo���Ă��܂��̂�����邽�߁j
		memcpy(tmpbuf, srcbuf, sizeof(uint8_t) * slen);
		tmpbuf[slen] = 0x00;
	}
	
	const uint8_t	*sbuf = (slen < MAXCONTROLSEQUENCE) ? tmpbuf : srcbuf;
	size_t			len = 0;

	switch(sbuf[0])
	{
		// �����T�C�Y�w��

		case 0x89:
			status->bNormalSize = false;					// �����T�C�Y���p(MSZ)�w��
			len = 1;
			break;
		case 0x8A:
			status->bNormalSize = true;						// �����T�C�Y�S�p(NSZ)�w��
			len = 1;
			break;

		// �����̌Ăяo��

		case 0x0F:											// LS0 (0F), G0->GL ���b�L���O�V�t�g
			status->region[REGION_GL]	= BANK_G0;
			len = 1;
			break;
		case 0x0E:											// LS1 (0E), G1->GL ���b�L���O�V�t�g
			status->region[REGION_GL]	= BANK_G1;
			len = 1;
			break;
		case 0x19:														// SS2 (19), G2->GL �V���O���V�t�g
			status->region_GL_backup	= status->region[REGION_GL];
			status->region[REGION_GL]	= BANK_G2;
			status->bSingleShift		= true;
			len = 1;
			break;
		case 0x1D:														// SS3 (1D), G3->GL �V���O���V�t�g
			status->region_GL_backup	= status->region[REGION_GL];
			status->region[REGION_GL]	= BANK_G3;
			status->bSingleShift		= true;
			len = 1;
			break;

		case 0x1B:		// ESC�ɑ�������R�[�h

			switch(sbuf[1])
			{
				// ESC�ɑ��������̌Ăяo��
				
				case 0x6E:									// LS2 (ESC 6E), G2->GL ���b�L���O�V�t�g
					status->region[REGION_GL] = BANK_G2;
					len = 2;
					break;
				case 0x6F:									// LS3 (ESC 6F), G3->GL ���b�L���O�V�t�g
					status->region[REGION_GL] = BANK_G3;
					len = 2;
					break;
				case 0x7E:									// LS1R (ESC 7E), G1->GR ���b�L���O�V�t�g
					status->region[REGION_GR] = BANK_G1;
					len = 2;
					break;
				case 0x7D:									// LS2R (ESC 7D), G2->GR ���b�L���O�V�t�g
					status->region[REGION_GR] = BANK_G2;
					len = 2;
					break;
				case 0x7C:									// LS3R (ESC 7C), G3->GR ���b�L���O�V�t�g
					status->region[REGION_GR] = BANK_G3;
					len = 2;
					break;

				// ESC�ɑ��������̎w������

				case 0x28:	// ESC 28
				case 0x29:	// ESC 29
				case 0x2A:	// ESC 2A
				case 0x2B:	// ESC 2B
					if(isOneByteGSET(sbuf[2]))
					{
						status->bank[numGBank(sbuf[1])] = sbuf[2];								// 1�o�C�gGSET�w�� (ESC 28|29|2A|2B [F]) -> G0,G1,G2,G3
						len = 3;
					} 
					else if(sbuf[2] == 0x20)
					{
						if(isOneByteDRCS(sbuf[3])) {
							status->bank[numGBank(sbuf[1])] = sbuf[3] + 0x100;					// 1�o�C�gDRCS�w�� (ESC 28|29|2A|2B 20 [F]) -> G0,G1,G2,G3		
							len = 4;															// + 0x100�͏I�[���������Ȃ��悤�ɂ��邽�߂̍׍H
						} else {
							len = 4;															// �s����1�o�C�gDRCS�w�� (ESC 28|29|2A|2B 20 XX)
						}
					}
					else
					{
						len = 3;																// �s����1�o�C�gGSET�w�� (ESC 28|29|2A|2B XX)
					}
					break;

				case 0x24:	// ESC 24
					if(isTwoByteGSET(sbuf[2]))
					{
						status->bank[BANK_G0] = sbuf[2];										// 2�o�C�gGSET�w�� (ESC 24 [F]) ->G0
						len = 3;
					}
					else
					{
						switch(sbuf[2])
						{
							case 0x28:	// ESC 24 28
								if(sbuf[3] == 0x20)
								{
									if(isTwoByteDRCS(sbuf[4])) {
										status->bank[BANK_G0] = sbuf[4];						// 2�o�C�gDRCS�w�� (ESC 24 28 20 [F]) ->G0
										len = 5;
									} else {
										len = 5;												// �s����2�o�C�gDRCS�w�� (ESC 24 28 20 XX)
									}
								}
								else
								{
									len = 4;													// �s���Ȏw�� (ESC 24 28 XX)
								}
								break;
							case 0x29:	// ESC 24 29
							case 0x2A:	// ESC 24 2A
							case 0x2B:	// ESC 24 2B
								if(isTwoByteGSET(sbuf[3]))
								{
									status->bank[numGBank(sbuf[2])] = sbuf[3];					// 2�o�C�gGSET�w�� (ESC 24 29|2A|2B [F]) ->G1,G2,G3
									len = 4;
								} 
								else if(sbuf[3] == 0x20)
								{
									if(isTwoByteDRCS(sbuf[4])) {
										status->bank[numGBank(sbuf[2])] = sbuf[4];				// 2�o�C�gDRCS�w�� (ESC 24 29|2A|2B 20 [F]) ->G1,G2,G3
										len = 5;
									} else {
										len = 5;												// �s����2�o�C�gDRCS�w�� (ESC 24 29|2A|2B 20 XX)
									}
								}
								else
								{
									len= 4;														// �s����2�o�C�gGSET�w�� (ESC 24 29|2A|2B XX)
								}
								break;
							default:
								len= 3;															// �s���Ȏw�� (ESC 24 XX)
						}
					}
					break;

				default:
					len = 2;																	// �s���Ȏw�� (ESC XX)
			}
			break;

		default:	// ��L�ȊO�̐���R�[�h
			len = 1;
	}

	return len;
}


size_t csiProc(const uint8_t *sbuf, size_t slen, ConvStatus *status)
{
// CSI����R�[�h����
// 
// XCS�̏����̂�

	int32_t		param[4];
	memset(param, 0, sizeof(param));	

	int32_t		numParam	= 0;
	bool		bParamEnd	= false;
	int32_t		charClass		=-1;
	
	size_t		src			= 1;
	
	while(src < slen)
	{
		if( isdigit(sbuf[src]) ) {											// �p�����[�^?
			param[numParam] = param[numParam] * 10 + (sbuf[src] - '0');
			src++;
		}
		else if(sbuf[src] == 0x3B) {										// �p�����[�^��؂�?
			numParam++;
			src++;
			if(numParam == 5) break;										// �p�����[�^������
		}
		else if(sbuf[src] == 0x20) {										// �p�����[�^�I��?
			numParam++;
			bParamEnd = true;
			src++;
		}
		else if( (sbuf[src] >= 0x42) && (sbuf[src] <= 0x6F) ) {				// �I�[����?
			charClass = sbuf[src];
			src++;
			break;
		}
		else {																// �s���ȃR�[�h
			src++;
			break;
		}
	}

	switch(charClass)
	{
		case	0x66:													// XCS
			if( bParamEnd && (numParam == 1) && (param[0] == 0) ) {
				status->bXCS = true;										// �O����֕�����`�J�n
				break;
			}
			if( bParamEnd && (numParam == 1) && (param[0] == 1) ) {
				status->bXCS = false;										// �O����֕�����`�I��
				break;
			}
			break;

		default:
			break;
	}

	return src;
}


void defaultMacroProc(const uint8_t c, ConvStatus *status)
{
// �f�t�H���g�}�N���̏���
// �ԑg���(SI)�ł͎g�p����Ȃ�

	switch(c)
	{
		case 0x60:
			status->bank[BANK_G0] = F_JIS1KANJI;
			status->bank[BANK_G1] = F_ALPHA;
			status->bank[BANK_G2] = F_HIRAGANA;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x61:
			status->bank[BANK_G0] = F_JIS1KANJI;
			status->bank[BANK_G1] = F_KATAKANA;
			status->bank[BANK_G2] = F_HIRAGANA;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x62:
			status->bank[BANK_G0] = F_JIS1KANJI;
			status->bank[BANK_G1] = F_DRCS1A;
			status->bank[BANK_G2] = F_HIRAGANA;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x63:
			status->bank[BANK_G0] = F_MOSAICA;
			status->bank[BANK_G1] = F_MOSAICC;
			status->bank[BANK_G2] = F_MOSAICD;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x64:
			status->bank[BANK_G0] = F_MOSAICA;
			status->bank[BANK_G1] = F_MOSAICB;
			status->bank[BANK_G2] = F_MOSAICD;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x65:
			status->bank[BANK_G0] = F_MOSAICA;
			status->bank[BANK_G1] = F_DRCS1A;
			status->bank[BANK_G2] = F_MOSAICD;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x66:
			status->bank[BANK_G0] = F_DRCS1A;
			status->bank[BANK_G1] = F_DRCS2A;
			status->bank[BANK_G2] = F_DRCS3A;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x67:
			status->bank[BANK_G0] = F_DRCS4A;
			status->bank[BANK_G1] = F_DRCS5A;
			status->bank[BANK_G2] = F_DRCS6A;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x68:
			status->bank[BANK_G0] = F_DRCS7A;
			status->bank[BANK_G1] = F_DRCS8A;
			status->bank[BANK_G2] = F_DRCS9A;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x69:
			status->bank[BANK_G0] = F_DRCS10A;
			status->bank[BANK_G1] = F_DRCS11A;
			status->bank[BANK_G2] = F_DRCS12A;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x6a:
			status->bank[BANK_G0] = F_DRCS13A;
			status->bank[BANK_G1] = F_DRCS14A;
			status->bank[BANK_G2] = F_DRCS15A;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x6b:
			status->bank[BANK_G0] = F_JIS1KANJI;
			status->bank[BANK_G1] = F_DRCS2A;
			status->bank[BANK_G2] = F_HIRAGANA;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x6c:
			status->bank[BANK_G0] = F_JIS1KANJI;
			status->bank[BANK_G1] = F_DRCS3A;
			status->bank[BANK_G2] = F_HIRAGANA;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x6d:
			status->bank[BANK_G0] = F_JIS1KANJI;
			status->bank[BANK_G1] = F_DRCS4A;
			status->bank[BANK_G2] = F_HIRAGANA;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x6e:
			status->bank[BANK_G0] = F_KATAKANA;
			status->bank[BANK_G1] = F_HIRAGANA;
			status->bank[BANK_G2] = F_ALPHA;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		case 0x6f:
			status->bank[BANK_G0] = F_ALPHA;
			status->bank[BANK_G1] = F_MOSAICA;
			status->bank[BANK_G2] = F_DRCS1A;
			status->bank[BANK_G3] = F_MACROA;
			status->region[REGION_GL] = BANK_G0;
			status->region[REGION_GR] = BANK_G2;
			break;
		default:
			break;
	}

	return;
}


int32_t classOfCharUTF16(const char16_t* srcbuf, const size_t slen, int32_t *jisCode, size_t *charLen)
{
//	unicode������𔻒肷��
//
//	sbuf		���f���镶��buf
//	slen		�ϊ���buf�̒���(char16_t�P��)	
//	�߂�l		���f���ʂ̕�����(F_CONTROL, F_ALPHA, �c)
//	jisCode		���ʂ�jiscode������
//	charLen		���ʂ�UTF-16������(char16_t�P��)������

	char16_t	cmpbuf[UTF16TABLELEN] = { 0 };

	if(slen < UTF16TABLELEN) {													// ��r�̕֋X�̂��߂Ƀo�b�t�@�ɃR�s�[����isbuf�͈̔͊O��ǂݏo���Ă��܂��̂�����邽�߁j
		memcpy(cmpbuf, srcbuf, sizeof(char16_t) * slen);
		cmpbuf[slen] = 0x0000;
	}

	const char16_t	*sbuf = (slen < UTF16TABLELEN) ? cmpbuf : srcbuf;


	size_t		clen	= 1;
	int32_t		uc		= sbuf[0];


	if( (uc >= 0xD800) && (uc <= 0xDBFF) )										// �T���Q�[�g�y�A�H
	{
		int32_t		uc1 = sbuf[1];
		if( (uc1 >= 0xDC00) && (uc1 <= 0xDFFF) ) {								// �T���Q�[�g�y�A�̓񕶎��ڃ`�F�b�N
			uc = ((uc - 0xD800) << 10) + (uc1 - 0xDC00) + 0x10000;
			clen = 2;
		} else {
			*jisCode = 0;
			*charLen = 1;
			return F_UNKNOWN;
		}
	}

	size_t		clen2	= 1;
	int32_t		uc2		= sbuf[clen];											// ���������ƈّ̎��Z���N�^�̃`�F�b�N�̂��߁A���̕������擾����

	if ((uc2 >= 0xD800) && (uc2 <= 0xDBFF))										// �T���Q�[�g�y�A�H
	{
		int32_t		uc3 = sbuf[clen + 1];
		if ((uc3 >= 0xDC00) && (uc3 <= 0xDFFF)) {								// �T���Q�[�g�y�A�̓񕶎��ڃ`�F�b�N
			uc2 = ((uc2 - 0xD800) << 10) + (uc3 - 0xDC00) + 0x10000;
			clen2 = 2;
		}
	}


	int32_t		code;
	size_t		len;
	int32_t		cType;


	// ����R�[�h (SP�܂�)

	if(isUcControlChar(uc)) {
		*jisCode	= uc;
		*charLen	= clen;
		return F_CONTROL;
	}


	// '['����n�܂�ǉ��L���W��
    
	if(uc == '[') {
		if( (len = kigou2RevConvUTF16(sbuf, UTF16TABLELEN, &code)) != 0) {
			*jisCode = code;
			*charLen = len;
			return F_KIGOU;
		}
	}


	// �p���W��
	
	code = alphaConv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_ALPHA;
	}


	// ��O�����񊿎���unicode���������ɊY���������
	// ��1,2��������, �ǉ������ّ̈̎��Z���N�^���g�p�������

	len = jis3CombAndIvsRevConv(uc, uc2, &cType, &code);
	if (len != 0) {
		*jisCode = code;
		*charLen = clen + clen2;
		return cType;
	}


	// �������W����Љ����W���̋��ʕ���

	code = kanaCommon1Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_KANACOMMON;
	}


	// �������W��

	code = hiragana1Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_HIRAGANA;
	}


	// �Љ����W��

	code = katakana1Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_KATAKANA;
	}


	// JIS X0201 �Љ����W��

	code = hankaku1Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_HANKAKU;
	}


	// �ǉ��L���W��

	code = kigou1Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_KIGOU;
	}


	// ��ꥑ�񐅏������W�� (�����W��)

	code = jis12Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_JIS1KANJI;
//		return F_KANJI;
	}


	// ��O���������W�� (JIS�݊�����1��)

	code = jis3Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_JIS1KANJI;
	}


	// ��l���������W�� (JIS�݊�����2��)

	code = jis4Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_JIS2KANJI;
	}


	// ���̑��̕s���ȕ���

	*jisCode = 0;
	*charLen = clen;
	return F_UNKNOWN;
}


int32_t classOfCharUTF8(const uint8_t* srcbuf, const size_t slen, int32_t *jisCode, size_t *charLen)
{
//	unicode������𔻒肷��
//
//	sbuf		���f���镶��buf
//	slen		�ϊ���buf�̒���(uint8_t�P��)	
//	�߂�l		���f���ʂ̕�����(F_CONTROL, F_ALPHA, �c)
//	jisCode		���ʂ�jiscode������
//	charLen		���ʂ�UTF-8������(uint8_t�P��)������

	uint8_t		cmpbuf[UTF8TABLELEN] = { 0 };

	if(slen < UTF8TABLELEN) {													// ��r�̕֋X�̂��߂Ƀo�b�t�@�ɃR�s�[����isbuf�͈̔͊O��ǂݏo���Ă��܂��̂�����邽�߁j
		memcpy(cmpbuf, srcbuf, sizeof(uint8_t) * slen);
		cmpbuf[slen] = 0x0000;
	}

	const uint8_t	*sbuf = (slen < UTF8TABLELEN) ? cmpbuf : srcbuf;


	int32_t		uc		= 0;
	size_t		clen	= 0;

	int32_t		firstbyte = sbuf[0];

	for(clen = 0; clen < 6;clen++) {					// �擪�o�C�g�̃r�b�g�p�^�[��(MSB�����'1'�̐��j�𒲂ׂ�
		if( (firstbyte & 0x80 ) == 0 ) break;
		firstbyte = firstbyte << 1;
	}
	
	switch(clen)
	{
		case 0:
			uc = sbuf[0] & 0x7F;
			clen++;
			break;
		case 2:
			uc = sbuf[0] & 0x1F;
			break;
		case 3:
			uc = sbuf[0] & 0x0F;
			break;
		case 4:
			uc = sbuf[0] & 0x07;
			break;
		case 1:
		case 5:
		default:
			*jisCode = 0;                                   
			*charLen = 1;
			return F_UNKNOWN;							// 1�̏ꍇ�͕����f�[�^�̓r��,5�ȏ�͕s���ȃf�[�^�Ƃ��Ăǂ������������
	}

	for(size_t i = 1; i < clen; i++) {
		if( (sbuf[i] & 0xC0) != 0x80 ) {				// ���������e�o�C�g��MSB��'10'�łȂ��Ȃ�s���ȃf�[�^
			*jisCode = 0;                                   
			*charLen = 1;
			return F_UNKNOWN;            
		}
		uc = (uc << 6) + (sbuf[i] & 0x3F);
	}

	int32_t		uc2		= 0;
	size_t		clen2	= 0;
	
	firstbyte = sbuf[clen];								// ���������ƈّ̎��Z���N�^�̃`�F�b�N�̂��߁A���̕������擾����

	for (clen2 = 0; clen2 < 6; clen2++) {				// �擪�o�C�g�̃r�b�g�p�^�[��(MSB�����'1'�̐��j�𒲂ׂ�
		if ((firstbyte & 0x80) == 0) break;
		firstbyte = firstbyte << 1;
	}

	switch (clen2)
	{
	case 0:
		uc2 = sbuf[clen] & 0x7F;
		clen2++;
		break;
	case 2:
		uc2 = sbuf[clen] & 0x1F;
		break;
	case 3:
		uc2 = sbuf[clen] & 0x0F;
		break;
	case 4:
		uc2 = sbuf[clen] & 0x07;
		break;
	case 1:
	case 5:
	default:
		uc2		= 0;
		clen2	= 1;									// 1�̏ꍇ�͕����f�[�^�̓r��,5�ȏ�͕s���ȃf�[�^�Ƃ��Ăǂ������������
	}

	for (size_t i = 1; i < clen2; i++) {
		if ((sbuf[clen + i] & 0xC0) != 0x80) {			// ���������e�o�C�g��MSB��'10'�łȂ��Ȃ�s���ȃf�[�^
			uc2 = 0;
			clen2 = 1;
			break;
		}
		uc2 = (uc2 << 6) + (sbuf[clen + i] & 0x3F);
	}


	int32_t		code;
	size_t		len;
	int32_t		cType;


	// ����R�[�h (SP�܂�)

	if(isUcControlChar(uc)) {
		*jisCode	= uc;
		*charLen	= clen;
		return F_CONTROL;
	}
    
    
	// '['����n�܂�ǉ��L���W��
    
	if(uc == '[') {
		if( (len = kigou2RevConvUTF8(sbuf, UTF8TABLELEN, &code)) != 0) {
			*jisCode = code;
			*charLen = len;
			return F_KIGOU;
		}
	}


	// �p���W��
	
	code = alphaConv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_ALPHA;
	}
    

	// ��O�����񊿎���unicode���������ɊY���������
	// ��1,2��������, �ǉ������ّ̈̎��Z���N�^���g�p�������

	len = jis3CombAndIvsRevConv(uc, uc2, &cType, &code);
	if (len != 0) {
		*jisCode = code;
		*charLen = clen + clen2;
		return cType;
	}


	// �������W����Љ����W���̋��ʕ���

	code = kanaCommon1Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_KANACOMMON;
	}


	// �������W��

	code = hiragana1Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_HIRAGANA;
	}


	// �Љ����W��

	code = katakana1Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_KATAKANA;
	}


	// JIS X0201 �Љ����W��

	code = hankaku1Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_HANKAKU;
	}


	// �ǉ��L���W��

	code = kigou1Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_KIGOU;
	}


	// ��ꥑ�񐅏������W�� (�����W��)

	code = jis12Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_JIS1KANJI;
//		return F_KANJI;
	}


	// ��O���������W�� (JIS�݊�����1��)

	code = jis3Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_JIS1KANJI;
	}


	// ��l���������W�� (JIS�݊�����2��)

	code = jis4Conv(uc, false);
	if(code != 0) {
		*jisCode = code;
		*charLen = clen;
		return F_JIS2KANJI;
	}


	// ���̑��̕s���ȕ���

	*jisCode = 0;
	*charLen = clen;
	return F_UNKNOWN;
}


uint32_t classOfCharU32T(const uint32_t code, int32_t *jisCode)
{
	//	unicode������𔻒肷��
	//
	//	code		���f���镶��32bit�����R�[�h	
	//	�߂�l		���f���ʂ̕�����(C_HALF_CONTROL, C_HALF_ALPHA, �c)
	//	jisCode		���ʂ�jiscode������
	//


	const uint32_t		c_code = code & 0xFFFF0000;
	const int32_t		jis    = code & 0x0000FFFF;

	// ����R�[�h (SP�܂�)

	switch (c_code)
	{
		case C_HALF_CONTROL:
		case C_FULL_CONTROL:
		case C_HALF_ALPHA:
		case C_FULL_ALPHA:
		case C_HALF_JIS1KANJI:
		case C_FULL_JIS1KANJI:
		case C_HALF_HIRAGANA:
		case C_FULL_HIRAGANA:
		case C_HALF_KATAKANA:
		case C_FULL_KATAKANA:
		case C_HALF_KANACOMMON:
		case C_FULL_KANACOMMON:
		case C_HALF_JIS2KANJI:
		case C_FULL_JIS2KANJI:
		case C_HALF_KIGOU:
		case C_FULL_KIGOU:
		case C_HALF_HANKAKU:
		case C_FULL_HANKAKU:
			*jisCode = jis;
			return c_code;
		default:
			*jisCode = 0;
			return C_UNKNOWN;
	}

}


void countMojiSequenceUTF16(const char16_t *sbuf, const size_t total_length, const ConvStatus *status, uint16_t *mojiClass, size_t *mojiNum, size_t *mojiLen, int32_t *numCType)
{
	// �\�[�X������̕����̕��я����擾���A���������J�E���g����D

	size_t		charLen = 0;
	int32_t		jisCode = 0;
	int32_t		charType = status->bank[status->region[REGION_GL]];
	int32_t		c;

	size_t		count = 0;

	mojiClass[count] = charType;
	mojiNum[count]   = 0;

	size_t		src = 0;

	while (src < total_length) {

		c = classOfCharUTF16(sbuf + src, total_length - src, &jisCode, &charLen);

		if ((c == F_JIS1KANJI) || (c == F_ALPHA) || (c == F_HIRAGANA) || (c == F_KATAKANA) || (c == F_KANACOMMON) || (c == F_KIGOU) || (c == F_HANKAKU) || (c == F_JIS2KANJI)) {
			if (c != charType) {
				count++;
				mojiClass[count] = c;
				mojiNum[count]   = 1;
				charType = c;
			}
			else {
				mojiNum[count]++;
			}
		}

		if ((c == F_KIGOU) && (*numCType < 5))     *numCType = 5;					// �ʏ�A������� F_JIS1KANJI, F_ALPHA, F_HIRAGANA, F_KATAKANA ��4��ނƂ��Ĉ������AF_KIGOU �����݂���ꍇ��5��ނƂ��Ĉ����D
		if ((c == F_HANKAKU) && (*numCType < 6))   *numCType = 6;					// F_HANKAKU �����݂���ꍇ��6��ނƂ��Ĉ����D
		if ((c == F_JIS2KANJI) && (*numCType < 7)) *numCType = 7;					// F_JIS2KANJI �����݂���ꍇ��7��ނƂ��Ĉ����D

		src += charLen;
	}

	count++;
	mojiClass[count] = F_NULL;
	mojiNum[count]   = 0;

	*mojiLen = count + 1;

	return;
}


void countMojiSequenceUTF8(const uint8_t *sbuf, const size_t total_length, const ConvStatus *status, uint16_t *mojiClass, size_t *mojiNum, size_t *mojiLen, int32_t *numCType)
{
	// �\�[�X������̕����̕��я����擾���A���������J�E���g����D

	size_t		charLen = 0;
	int32_t		jisCode = 0;
	int32_t		charType = status->bank[status->region[REGION_GL]];
	int32_t		c;

	size_t		count = 0;

	mojiClass[count] = charType;
	mojiNum[count] = 0;

	size_t		src = 0;

	while (src < total_length) {

		c = classOfCharUTF8(sbuf + src, total_length - src, &jisCode, &charLen);

		if ((c == F_JIS1KANJI) || (c == F_ALPHA) || (c == F_HIRAGANA) || (c == F_KATAKANA) || (c == F_KANACOMMON) || (c == F_KIGOU) || (c == F_HANKAKU) || (c == F_JIS2KANJI)) {
			if (c != charType) {
				count++;
				mojiClass[count] = c;
				mojiNum[count] = 1;
				charType = c;
			}
			else {
				mojiNum[count]++;
			}
		}

		if ((c == F_KIGOU) && (*numCType < 5))     *numCType = 5;					// �ʏ�A������� F_JIS1KANJI, F_ALPHA, F_HIRAGANA, F_KATAKANA ��4��ނƂ��Ĉ������AF_KIGOU �����݂���ꍇ��5��ނƂ��Ĉ����D
		if ((c == F_HANKAKU) && (*numCType < 6))   *numCType = 6;					// F_HANKAKU �����݂���ꍇ��6��ނƂ��Ĉ����D
		if ((c == F_JIS2KANJI) && (*numCType < 7)) *numCType = 7;					// F_JIS2KANJI �����݂���ꍇ��7��ނƂ��Ĉ����D

		src += charLen;
	}

	count++;
	mojiClass[count] = F_NULL;
	mojiNum[count] = 0;

	*mojiLen = count + 1;

	return;
}


void countMojiSequenceU32T(const uint32_t *sbuf, const size_t total_length, const ConvStatus *status, uint16_t *mojiClass, size_t *mojiNum, size_t *mojiLen, int32_t *numCType)
{
	// ���������pU32T������̕����̕��я����擾���A���������J�E���g����D

	int32_t		jisCode = 0;
	int32_t		charType = status->bank[status->region[REGION_GL]];
	int32_t		c;

	size_t		count = 0;

	mojiClass[count] = charType;
	mojiNum[count] = 0;

	size_t		src = 0;

	while (src < total_length) {

		c = (classOfCharU32T(sbuf[src], &jisCode) & C_MASK_CHARCODE) >> 16;

		if ((c == F_JIS1KANJI) || (c == F_ALPHA) || (c == F_HIRAGANA) || (c == F_KATAKANA) || (c == F_KANACOMMON) || (c == F_KIGOU) || (c == F_HANKAKU) || (c == F_JIS2KANJI)) {
			if (c != charType) {
				count++;
				mojiClass[count] = c;
				mojiNum[count] = 1;
				charType = c;
			}
			else {
				mojiNum[count]++;
			}
		}

		if ((c == F_KIGOU) && (*numCType < 5))     *numCType = 5;					// �ʏ�A������� F_JIS1KANJI, F_ALPHA, F_HIRAGANA, F_KATAKANA ��4��ނƂ��Ĉ������AF_KIGOU �����݂���ꍇ��5��ނƂ��Ĉ����D
		if ((c == F_HANKAKU) && (*numCType < 6))   *numCType = 6;					// F_HANKAKU �����݂���ꍇ��6��ނƂ��Ĉ����D
		if ((c == F_JIS2KANJI) && (*numCType < 7)) *numCType = 7;					// F_JIS2KANJI �����݂���ꍇ��7��ނƂ��Ĉ����D

		src++;
	}

	count++;
	mojiClass[count] = F_NULL;
	mojiNum[count] = 0;

	*mojiLen = count + 1;

	return;
}


int32_t getCTypeCode(const int32_t cType)
{
	switch (cType)
	{
		case F_JIS1KANJI:	return 0;
		case F_ALPHA:		return 1;
		case F_HIRAGANA:	return 2;
		case F_KATAKANA:	return 3;
		case F_KIGOU:		return 4;
		case F_HANKAKU:		return 5;
		case F_JIS2KANJI:	return 6;
		default:
			break;
	}

	return 6;
}


void initBankElementSeq(BankUnit &src, const int32_t numCType, const size_t bsize)
{
	src.group[0].numCType = numCType;
	src.group[1].numCType = numCType;
	src.group[2].numCType = numCType;

	for (int32_t x = 0; x < 3; x++)
	{
		for (int32_t i = 0; i < numCType; i++)
		{
			for (int32_t j = 0; j < numCType; j++)
			{
				if (i == j) continue;
				for (int32_t k = 0; k < numCType; k++)
				{
					if ((i == k) || (j == k)) continue;

					src.group[x].element0[0][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element0[1][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element0[2][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element1[0][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element1[1][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element2[0][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element2[1][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element2[2][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element3[0][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element3[1][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element4[0][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element4[1][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element4[2][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element5[0][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element5[1][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element5[2][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element5[3][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element6[0][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element6[1][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element7[0][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element7[1][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element7[2][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element8[0][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element8[1][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element8[2][i][j][k].bseq = new uint8_t [bsize];
					src.group[x].element8[3][i][j][k].bseq = new uint8_t [bsize];
				}
			}
		}
	}

	return;
}


void deleteBankElementSeq(BankUnit &src)
{
	const int32_t	numCType = src.group[0].numCType;

	for (int32_t x = 0; x < 3; x++)
	{
		for (int32_t i = 0; i < numCType; i++)
		{
			for (int32_t j = 0; j < numCType; j++)
			{
				if (i == j) continue;
				for (int32_t k = 0; k < numCType; k++)
				{
					if ((i == k) || (j == k)) continue;

					delete [] src.group[x].element0[0][i][j][k].bseq;
					delete [] src.group[x].element0[1][i][j][k].bseq;
					delete [] src.group[x].element0[2][i][j][k].bseq;
					delete [] src.group[x].element1[0][i][j][k].bseq;
					delete [] src.group[x].element1[1][i][j][k].bseq;
					delete [] src.group[x].element2[0][i][j][k].bseq;
					delete [] src.group[x].element2[1][i][j][k].bseq;
					delete [] src.group[x].element2[2][i][j][k].bseq;
					delete [] src.group[x].element3[0][i][j][k].bseq;
					delete [] src.group[x].element3[1][i][j][k].bseq;
					delete [] src.group[x].element4[0][i][j][k].bseq;
					delete [] src.group[x].element4[1][i][j][k].bseq;
					delete [] src.group[x].element4[2][i][j][k].bseq;
					delete [] src.group[x].element5[0][i][j][k].bseq;
					delete [] src.group[x].element5[1][i][j][k].bseq;
					delete [] src.group[x].element5[2][i][j][k].bseq;
					delete [] src.group[x].element5[3][i][j][k].bseq;
					delete [] src.group[x].element6[0][i][j][k].bseq;
					delete [] src.group[x].element6[1][i][j][k].bseq;
					delete [] src.group[x].element7[0][i][j][k].bseq;
					delete [] src.group[x].element7[1][i][j][k].bseq;
					delete [] src.group[x].element7[2][i][j][k].bseq;
					delete [] src.group[x].element8[0][i][j][k].bseq;
					delete [] src.group[x].element8[1][i][j][k].bseq;
					delete [] src.group[x].element8[2][i][j][k].bseq;
					delete [] src.group[x].element8[3][i][j][k].bseq;
				}
			}
		}
	}

	return;
}


inline void initBankElement(BankElement *element, const size_t cost, const BankSet bank)
{
	element->cost = cost;
	element->tmpcost = SIZE_MAX;
	element->next = NULL;
	element->blen = 1;
	element->bseq[0] = bank;

	return;
}


void initBankUnit(BankUnit *unit, const int32_t cType, const size_t cNum)
{
	//
	// �ʏ�̕�����(F_JIS1KANJI, F_ALPHA, F_KIGOU, F_HANKAKU, F_JIS2KANJI)�͒P�Ƃ̕�����Ƃ��Ĉ����D
	// F_HIRAGANA�́AF_HIRAGANA, F_JIS1KANJI�̑I�����̑g�ݍ��킹�Ƃ��Ĉ����D
	// F_KATAKANA�́AF_KATAKANA, F_JIS1KANJI�̑I�����̑g�ݍ��킹�Ƃ��Ĉ����D
	// F_KANACOMMON�́AF_HIRAGANA, F_JIS1KANJI, F_KATAKANA�̑I�����̑g�ݍ��킹�Ƃ��Ĉ����D
	//

	unit->cType = cType;
	unit->cNum = cNum;

	if (cType == F_NULL) return;

	if (cType != F_KANACOMMON)
	{
		const size_t		cSize = isTwoByteGSET(cType) ? 2 : 1;

		unit->group[0].cType = cType;
		unit->group[0].cCode = getCTypeCode(cType);

		for (int32_t i = 0; i < unit->group[0].numCType; i++)
		{
			if (i == unit->group[0].cCode) continue;
			for (int32_t j = 0; j < unit->group[0].numCType; j++)
			{
				if ((j == unit->group[0].cCode) || (j == i)) continue;
				for (int32_t k = 0; k < unit->group[0].numCType; k++)
				{
					if ((k == unit->group[0].cCode) || (k == i) || (k == j)) continue;

					initBankElement(&unit->group[0].element0[0][i][j][k], cSize * cNum, USE_BANK_G0_TO_GL);
					initBankElement(&unit->group[0].element0[1][i][j][k], cSize * cNum, USE_BANK_G0_TO_GL);
					initBankElement(&unit->group[0].element0[2][i][j][k], cSize * cNum, USE_BANK_G0_TO_GL);

					initBankElement(&unit->group[0].element1[0][i][j][k], cSize * cNum, USE_BANK_G1_TO_GL);
					initBankElement(&unit->group[0].element1[1][i][j][k], cSize * cNum, USE_BANK_G1_TO_GL);

					initBankElement(&unit->group[0].element2[0][i][j][k], cSize * cNum, USE_BANK_G1_TO_GR);
					initBankElement(&unit->group[0].element2[1][i][j][k], cSize * cNum, USE_BANK_G1_TO_GR);
					initBankElement(&unit->group[0].element2[2][i][j][k], cSize * cNum, USE_BANK_G1_TO_GR);

					initBankElement(&unit->group[0].element3[0][i][j][k], cSize * cNum, USE_BANK_G2_TO_GL);
					initBankElement(&unit->group[0].element3[1][i][j][k], cSize * cNum, USE_BANK_G2_TO_GL);

					initBankElement(&unit->group[0].element4[0][i][j][k], cSize * cNum, USE_BANK_G2_TO_GR);
					initBankElement(&unit->group[0].element4[1][i][j][k], cSize * cNum, USE_BANK_G2_TO_GR);
					initBankElement(&unit->group[0].element4[2][i][j][k], cSize * cNum, USE_BANK_G2_TO_GR);

					initBankElement(&unit->group[0].element5[0][i][j][k], (cSize + 1) * cNum, USE_BANK_G2_SSHIFT);
					initBankElement(&unit->group[0].element5[1][i][j][k], (cSize + 1) * cNum, USE_BANK_G2_SSHIFT);
					initBankElement(&unit->group[0].element5[2][i][j][k], (cSize + 1) * cNum, USE_BANK_G2_SSHIFT);
					initBankElement(&unit->group[0].element5[3][i][j][k], (cSize + 1) * cNum, USE_BANK_G2_SSHIFT);

					initBankElement(&unit->group[0].element6[0][i][j][k], cSize * cNum, USE_BANK_G3_TO_GL);
					initBankElement(&unit->group[0].element6[1][i][j][k], cSize * cNum, USE_BANK_G3_TO_GL);

					initBankElement(&unit->group[0].element7[0][i][j][k], cSize * cNum, USE_BANK_G3_TO_GR);
					initBankElement(&unit->group[0].element7[1][i][j][k], cSize * cNum, USE_BANK_G3_TO_GR);
					initBankElement(&unit->group[0].element7[2][i][j][k], cSize * cNum, USE_BANK_G3_TO_GR);

					initBankElement(&unit->group[0].element8[0][i][j][k], (cSize + 1) * cNum, USE_BANK_G3_SSHIFT);
					initBankElement(&unit->group[0].element8[1][i][j][k], (cSize + 1) * cNum, USE_BANK_G3_SSHIFT);
					initBankElement(&unit->group[0].element8[2][i][j][k], (cSize + 1) * cNum, USE_BANK_G3_SSHIFT);
					initBankElement(&unit->group[0].element8[3][i][j][k], (cSize + 1) * cNum, USE_BANK_G3_SSHIFT);
				}
			}
		}
	}
	else		// cType == F_KANACOMMON
	{
		const size_t		cSize = 1;

		unit->group[0].cType = F_HIRAGANA;
		unit->group[0].cCode = getCTypeCode(F_HIRAGANA);

		for (int32_t i = 0; i < unit->group[0].numCType; i++)
		{
			if (i == unit->group[0].cCode) continue;
			for (int32_t j = 0; j < unit->group[0].numCType; j++)
			{
				if ((j == unit->group[0].cCode) || (j == i)) continue;
				for (int32_t k = 0; k < unit->group[0].numCType; k++)
				{
					if ((k == unit->group[0].cCode) || (k == i) || (k == j)) continue;

					initBankElement(&unit->group[0].element0[0][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G0_TO_GL);
					initBankElement(&unit->group[0].element0[1][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G0_TO_GL);
					initBankElement(&unit->group[0].element0[2][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G0_TO_GL);

					initBankElement(&unit->group[0].element1[0][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G1_TO_GL);
					initBankElement(&unit->group[0].element1[1][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G1_TO_GL);

					initBankElement(&unit->group[0].element2[0][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G1_TO_GR);
					initBankElement(&unit->group[0].element2[1][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G1_TO_GR);
					initBankElement(&unit->group[0].element2[2][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G1_TO_GR);

					initBankElement(&unit->group[0].element3[0][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G2_TO_GL);
					initBankElement(&unit->group[0].element3[1][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G2_TO_GL);

					initBankElement(&unit->group[0].element4[0][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G2_TO_GR);
					initBankElement(&unit->group[0].element4[1][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G2_TO_GR);
					initBankElement(&unit->group[0].element4[2][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G2_TO_GR);

					initBankElement(&unit->group[0].element5[0][i][j][k], (cSize + 1) * cNum, USE_HIRAGANA_BANK_G2_SSHIFT);
					initBankElement(&unit->group[0].element5[1][i][j][k], (cSize + 1) * cNum, USE_HIRAGANA_BANK_G2_SSHIFT);
					initBankElement(&unit->group[0].element5[2][i][j][k], (cSize + 1) * cNum, USE_HIRAGANA_BANK_G2_SSHIFT);
					initBankElement(&unit->group[0].element5[3][i][j][k], (cSize + 1) * cNum, USE_HIRAGANA_BANK_G2_SSHIFT);

					initBankElement(&unit->group[0].element6[0][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G3_TO_GL);
					initBankElement(&unit->group[0].element6[1][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G3_TO_GL);

					initBankElement(&unit->group[0].element7[0][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G3_TO_GR);
					initBankElement(&unit->group[0].element7[1][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G3_TO_GR);
					initBankElement(&unit->group[0].element7[2][i][j][k], cSize * cNum, USE_HIRAGANA_BANK_G3_TO_GR);

					initBankElement(&unit->group[0].element8[0][i][j][k], (cSize + 1) * cNum, USE_HIRAGANA_BANK_G3_SSHIFT);
					initBankElement(&unit->group[0].element8[1][i][j][k], (cSize + 1) * cNum, USE_HIRAGANA_BANK_G3_SSHIFT);
					initBankElement(&unit->group[0].element8[2][i][j][k], (cSize + 1) * cNum, USE_HIRAGANA_BANK_G3_SSHIFT);
					initBankElement(&unit->group[0].element8[3][i][j][k], (cSize + 1) * cNum, USE_HIRAGANA_BANK_G3_SSHIFT);
				}
			}
		}
	}

	if ((cType == F_HIRAGANA) || (cType == F_KATAKANA) || (cType == F_KANACOMMON))
	{
		const size_t		cSize = 2;

		unit->group[1].cType = F_JIS1KANJI;
		unit->group[1].cCode = getCTypeCode(F_JIS1KANJI);

		for (int32_t i = 0; i < unit->group[1].numCType; i++)
		{
			if (i == unit->group[1].cCode) continue;
			for (int32_t j = 0; j < unit->group[1].numCType; j++)
			{
				if ((j == unit->group[1].cCode) || (j == i)) continue;
				for (int32_t k = 0; k < unit->group[1].numCType; k++)
				{
					if ((k == unit->group[1].cCode) || (k == i) || (k == j)) continue;

					initBankElement(&unit->group[1].element0[0][i][j][k], cSize * cNum, USE_KANJI_BANK_G0_TO_GL);
					initBankElement(&unit->group[1].element0[1][i][j][k], cSize * cNum, USE_KANJI_BANK_G0_TO_GL);
					initBankElement(&unit->group[1].element0[2][i][j][k], cSize * cNum, USE_KANJI_BANK_G0_TO_GL);

					initBankElement(&unit->group[1].element1[0][i][j][k], cSize * cNum, USE_KANJI_BANK_G1_TO_GL);
					initBankElement(&unit->group[1].element1[1][i][j][k], cSize * cNum, USE_KANJI_BANK_G1_TO_GL);

					initBankElement(&unit->group[1].element2[0][i][j][k], cSize * cNum, USE_KANJI_BANK_G1_TO_GR);
					initBankElement(&unit->group[1].element2[1][i][j][k], cSize * cNum, USE_KANJI_BANK_G1_TO_GR);
					initBankElement(&unit->group[1].element2[2][i][j][k], cSize * cNum, USE_KANJI_BANK_G1_TO_GR);

					initBankElement(&unit->group[1].element3[0][i][j][k], cSize * cNum, USE_KANJI_BANK_G2_TO_GL);
					initBankElement(&unit->group[1].element3[1][i][j][k], cSize * cNum, USE_KANJI_BANK_G2_TO_GL);

					initBankElement(&unit->group[1].element4[0][i][j][k], cSize * cNum, USE_KANJI_BANK_G2_TO_GR);
					initBankElement(&unit->group[1].element4[1][i][j][k], cSize * cNum, USE_KANJI_BANK_G2_TO_GR);
					initBankElement(&unit->group[1].element4[2][i][j][k], cSize * cNum, USE_KANJI_BANK_G2_TO_GR);

					initBankElement(&unit->group[1].element5[0][i][j][k], (cSize + 1) * cNum, USE_KANJI_BANK_G2_SSHIFT);
					initBankElement(&unit->group[1].element5[1][i][j][k], (cSize + 1) * cNum, USE_KANJI_BANK_G2_SSHIFT);
					initBankElement(&unit->group[1].element5[2][i][j][k], (cSize + 1) * cNum, USE_KANJI_BANK_G2_SSHIFT);
					initBankElement(&unit->group[1].element5[3][i][j][k], (cSize + 1) * cNum, USE_KANJI_BANK_G2_SSHIFT);

					initBankElement(&unit->group[1].element6[0][i][j][k], cSize * cNum, USE_KANJI_BANK_G3_TO_GL);
					initBankElement(&unit->group[1].element6[1][i][j][k], cSize * cNum, USE_KANJI_BANK_G3_TO_GL);

					initBankElement(&unit->group[1].element7[0][i][j][k], cSize * cNum, USE_KANJI_BANK_G3_TO_GR);
					initBankElement(&unit->group[1].element7[1][i][j][k], cSize * cNum, USE_KANJI_BANK_G3_TO_GR);
					initBankElement(&unit->group[1].element7[2][i][j][k], cSize * cNum, USE_KANJI_BANK_G3_TO_GR);

					initBankElement(&unit->group[1].element8[0][i][j][k], (cSize + 1) * cNum, USE_KANJI_BANK_G3_SSHIFT);
					initBankElement(&unit->group[1].element8[1][i][j][k], (cSize + 1) * cNum, USE_KANJI_BANK_G3_SSHIFT);
					initBankElement(&unit->group[1].element8[2][i][j][k], (cSize + 1) * cNum, USE_KANJI_BANK_G3_SSHIFT);
					initBankElement(&unit->group[1].element8[3][i][j][k], (cSize + 1) * cNum, USE_KANJI_BANK_G3_SSHIFT);
				}
			}
		}
	}

	if (cType == F_KANACOMMON)
	{
		const size_t		cSize = 1;

		unit->group[2].cType = F_KATAKANA;
		unit->group[2].cCode = getCTypeCode(F_KATAKANA);

		for (int32_t i = 0; i < unit->group[2].numCType; i++)
		{
			if (i == unit->group[2].cCode) continue;
			for (int32_t j = 0; j < unit->group[2].numCType; j++)
			{
				if ((j == unit->group[2].cCode) || (j == i)) continue;
				for (int32_t k = 0; k < unit->group[2].numCType; k++)
				{
					if ((k == unit->group[2].cCode) || (k == i) || (k == j)) continue;

					initBankElement(&unit->group[2].element0[0][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G0_TO_GL);
					initBankElement(&unit->group[2].element0[1][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G0_TO_GL);
					initBankElement(&unit->group[2].element0[2][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G0_TO_GL);

					initBankElement(&unit->group[2].element1[0][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G1_TO_GL);
					initBankElement(&unit->group[2].element1[1][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G1_TO_GL);

					initBankElement(&unit->group[2].element2[0][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G1_TO_GR);
					initBankElement(&unit->group[2].element2[1][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G1_TO_GR);
					initBankElement(&unit->group[2].element2[2][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G1_TO_GR);

					initBankElement(&unit->group[2].element3[0][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G2_TO_GL);
					initBankElement(&unit->group[2].element3[1][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G2_TO_GL);

					initBankElement(&unit->group[2].element4[0][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G2_TO_GR);
					initBankElement(&unit->group[2].element4[1][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G2_TO_GR);
					initBankElement(&unit->group[2].element4[2][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G2_TO_GR);

					initBankElement(&unit->group[2].element5[0][i][j][k], (cSize + 1) * cNum, USE_KATAKANA_BANK_G2_SSHIFT);
					initBankElement(&unit->group[2].element5[1][i][j][k], (cSize + 1) * cNum, USE_KATAKANA_BANK_G2_SSHIFT);
					initBankElement(&unit->group[2].element5[2][i][j][k], (cSize + 1) * cNum, USE_KATAKANA_BANK_G2_SSHIFT);
					initBankElement(&unit->group[2].element5[3][i][j][k], (cSize + 1) * cNum, USE_KATAKANA_BANK_G2_SSHIFT);

					initBankElement(&unit->group[2].element6[0][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G3_TO_GL);
					initBankElement(&unit->group[2].element6[1][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G3_TO_GL);

					initBankElement(&unit->group[2].element7[0][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G3_TO_GR);
					initBankElement(&unit->group[2].element7[1][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G3_TO_GR);
					initBankElement(&unit->group[2].element7[2][i][j][k], cSize * cNum, USE_KATAKANA_BANK_G3_TO_GR);

					initBankElement(&unit->group[2].element8[0][i][j][k], (cSize + 1) * cNum, USE_KATAKANA_BANK_G3_SSHIFT);
					initBankElement(&unit->group[2].element8[1][i][j][k], (cSize + 1) * cNum, USE_KATAKANA_BANK_G3_SSHIFT);
					initBankElement(&unit->group[2].element8[2][i][j][k], (cSize + 1) * cNum, USE_KATAKANA_BANK_G3_SSHIFT);
					initBankElement(&unit->group[2].element8[3][i][j][k], (cSize + 1) * cNum, USE_KATAKANA_BANK_G3_SSHIFT);
				}
			}
		}
	}

	return;
}


inline void checkBankElement(BankElement &src, BankElement &dst, const int32_t additionalcost)
{
	if (src.tmpcost > (dst.cost + additionalcost)) {
		src.tmpcost = dst.cost + additionalcost;
		src.next = &dst;
	}

	return;
}


inline void processBankElement(BankElement &src)
{
	if (src.next != NULL) {
		src.blen += src.next->blen;
		src.bseq[src.blen - 1] = src.bseq[0];
		memcpy(src.bseq, src.next->bseq, src.blen - 1);
		src.cost += src.tmpcost;
	}

	return;
}


void processBankGroup(BankGroup &src)
{
	for (int32_t i = 0; i < src.numCType; i++)
	{
		if (i == src.cCode) continue;
		for (int32_t j = 0; j < src.numCType; j++)
		{
			if ((j == src.cCode) || (j == i)) continue;
			for (int32_t k = 0; k < src.numCType; k++)
			{
				if ((k == src.cCode) || (k == i) || (k == j)) continue;

				processBankElement(src.element0[0][i][j][k]);
				processBankElement(src.element0[1][i][j][k]);
				processBankElement(src.element0[2][i][j][k]);
				processBankElement(src.element1[0][i][j][k]);
				processBankElement(src.element1[1][i][j][k]);
				processBankElement(src.element2[0][i][j][k]);
				processBankElement(src.element2[1][i][j][k]);
				processBankElement(src.element2[2][i][j][k]);
				processBankElement(src.element3[0][i][j][k]);
				processBankElement(src.element3[1][i][j][k]);
				processBankElement(src.element4[0][i][j][k]);
				processBankElement(src.element4[1][i][j][k]);
				processBankElement(src.element4[2][i][j][k]);
				processBankElement(src.element5[0][i][j][k]);
				processBankElement(src.element5[1][i][j][k]);
				processBankElement(src.element5[2][i][j][k]);
				processBankElement(src.element5[3][i][j][k]);
				processBankElement(src.element6[0][i][j][k]);
				processBankElement(src.element6[1][i][j][k]);
				processBankElement(src.element7[0][i][j][k]);
				processBankElement(src.element7[1][i][j][k]);
				processBankElement(src.element7[2][i][j][k]);
				processBankElement(src.element8[0][i][j][k]);
				processBankElement(src.element8[1][i][j][k]);
				processBankElement(src.element8[2][i][j][k]);
				processBankElement(src.element8[3][i][j][k]);
			}
		}
	}

	return;
}


void calcBankGroupCost(BankGroup &src, BankGroup &dst)
{
	//
	// src -> dst�ɏ�Ԃ��ς��ۂ̃R�X�g���v�Z���A�I�����̓��́A�ł��R�X�g�̏��������[�g��I������D
	//

	if (src.cType == dst.cType)								// src.cType == dst.cType (�����킪����)�̏ꍇ
	{
		for (int32_t i = 0; i < src.numCType; i++)
		{
			if (i == src.cCode) continue;
			for (int32_t j = 0; j < src.numCType; j++)
			{
				if ((j == src.cCode) || (j == i)) continue;
				for (int32_t k = 0; k < src.numCType; k++)
				{
					if ((k == src.cCode) || (k == i) || (k == j)) continue;

					checkBankElement(src.element0[0][i][j][k], dst.element0[0][i][j][k], 0);
					checkBankElement(src.element0[1][i][j][k], dst.element0[1][i][j][k], 0);
					checkBankElement(src.element0[2][i][j][k], dst.element0[2][i][j][k], 0);

					checkBankElement(src.element1[0][i][j][k], dst.element1[0][i][j][k], 0);
					checkBankElement(src.element1[1][i][j][k], dst.element1[1][i][j][k], 0);

					checkBankElement(src.element2[0][i][j][k], dst.element2[0][i][j][k], 0);
					checkBankElement(src.element2[1][i][j][k], dst.element2[1][i][j][k], 0);
					checkBankElement(src.element2[2][i][j][k], dst.element2[2][i][j][k], 0);

					checkBankElement(src.element3[0][i][j][k], dst.element3[0][i][j][k], 0);
					checkBankElement(src.element3[1][i][j][k], dst.element3[1][i][j][k], 0);

					checkBankElement(src.element4[0][i][j][k], dst.element4[0][i][j][k], 0);
					checkBankElement(src.element4[1][i][j][k], dst.element4[1][i][j][k], 0);
					checkBankElement(src.element4[2][i][j][k], dst.element4[2][i][j][k], 0);

					checkBankElement(src.element5[0][i][j][k], dst.element5[0][i][j][k], 0);
					checkBankElement(src.element5[1][i][j][k], dst.element5[1][i][j][k], 0);
					checkBankElement(src.element5[2][i][j][k], dst.element5[2][i][j][k], 0);
					checkBankElement(src.element5[3][i][j][k], dst.element5[3][i][j][k], 0);

					checkBankElement(src.element6[0][i][j][k], dst.element6[0][i][j][k], 0);
					checkBankElement(src.element6[1][i][j][k], dst.element6[1][i][j][k], 0);

					checkBankElement(src.element7[0][i][j][k], dst.element7[0][i][j][k], 0);
					checkBankElement(src.element7[1][i][j][k], dst.element7[1][i][j][k], 0);
					checkBankElement(src.element7[2][i][j][k], dst.element7[2][i][j][k], 0);

					checkBankElement(src.element8[0][i][j][k], dst.element8[0][i][j][k], 0);
					checkBankElement(src.element8[1][i][j][k], dst.element8[1][i][j][k], 0);
					checkBankElement(src.element8[2][i][j][k], dst.element8[2][i][j][k], 0);
					checkBankElement(src.element8[3][i][j][k], dst.element8[3][i][j][k], 0);

				}
			}
		}

		return;
	}

	const int32_t	cost_set_g0 = COST_GSET_TO_G0;															// BANK_G0 �̕������ύX����R�X�g
	const int32_t	cost_set_g1 = isTwoByteGSET(dst.cType) ? COST_2GSET_TO_G1 : COST_1GSET_TO_G1;			// BANK_G1 �̕������ύX����R�X�g
	const int32_t	cost_set_g2 = isTwoByteGSET(dst.cType) ? COST_2GSET_TO_G2 : COST_1GSET_TO_G2;			// BANK_G2 �̕������ύX����R�X�g
	const int32_t	cost_set_g3 = isTwoByteGSET(dst.cType) ? COST_2GSET_TO_G3 : COST_1GSET_TO_G3;			// BANK_G3 �̕������ύX����R�X�g


	//

	for (int32_t i = 0; i < src.numCType; i++)
	{
		if (i == src.cCode) continue;
		for (int32_t j = 0; j < src.numCType; j++)
		{
			if ((j == src.cCode) || (j == i)) continue;
			for (int32_t k = 0; k < src.numCType; k++)
			{
				if ((k == src.cCode) || (k == i) || (k == j)) continue;

				// element0[0]: �Ώە����킪 BANK_G0 �ɂ���A�����REGION_GL �ŕ\�����Ă����� (G0->GL, G1->GR, G0=src.cType, G1=i, G2=j, G3=k) -------------------------------------------
				
				int32_t		tmp_cost_set_g0 = cost_set_g0;
				int32_t		tmp_cost_set_g1 = cost_set_g1;
				int32_t		tmp_cost_set_g2 = cost_set_g2;
				int32_t		tmp_cost_set_g3 = cost_set_g3;

				BankRouteSelection	sel;
				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G1 �� dst.cType �̏ꍇ
					sel.b2 = true;
					tmp_cost_set_g1 = 0;
				} 
				else if (j == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = sel.b4 = sel.b5 = true;
					tmp_cost_set_g2 = 0;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = sel.b7 = sel.b8 = true;
					tmp_cost_set_g3 = 0;
				}
				else {
					sel.b0 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element0[0][i][j][k], dst.element0[0][i][j][k],         tmp_cost_set_g0);
//				if (sel.b1)
				if (sel.b2) checkBankElement(src.element0[0][i][j][k], dst.element2[0][src.cCode][j][k], tmp_cost_set_g1);
				if (sel.b3) checkBankElement(src.element0[0][i][j][k], dst.element3[0][src.cCode][i][k], tmp_cost_set_g2 + COST_G2_LS2);
				if (sel.b4) checkBankElement(src.element0[0][i][j][k], dst.element4[0][src.cCode][i][k], tmp_cost_set_g2 + COST_G2_LS2R);
				if (sel.b5) checkBankElement(src.element0[0][i][j][k], dst.element5[0][src.cCode][i][k], tmp_cost_set_g2);
				if (sel.b6) checkBankElement(src.element0[0][i][j][k], dst.element6[0][src.cCode][i][j], tmp_cost_set_g3 + COST_G3_LS3);
				if (sel.b7) checkBankElement(src.element0[0][i][j][k], dst.element7[0][src.cCode][i][j], tmp_cost_set_g3 + COST_G3_LS3R);
				if (sel.b8) checkBankElement(src.element0[0][i][j][k], dst.element8[0][src.cCode][i][j], tmp_cost_set_g3);

				// element0[1]: �Ώە����킪 BANK_G0 �ɂ���A�����REGION_GL �ŕ\�����Ă����� (G0->GL, G2->GR, G0=src.cType, G1=i, G2=j, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = sel.b2 = true;
				}
				else if (j == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b4 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = sel.b7 = sel.b8 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b2 = sel.b4 = sel.b6 = sel.b7 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element0[1][i][j][k], dst.element0[1][i][j][k],         tmp_cost_set_g0);
				if (sel.b1) checkBankElement(src.element0[1][i][j][k], dst.element1[0][src.cCode][j][k], tmp_cost_set_g1 + COST_G1_LS1);
				if (sel.b2) checkBankElement(src.element0[1][i][j][k], dst.element2[0][src.cCode][j][k], tmp_cost_set_g1 + COST_G1_LS1R);
//				if (sel.b3)
				if (sel.b4) checkBankElement(src.element0[1][i][j][k], dst.element4[0][src.cCode][i][k], tmp_cost_set_g2);
//				if (sel.b5)
				if (sel.b6) checkBankElement(src.element0[1][i][j][k], dst.element6[1][src.cCode][i][j], tmp_cost_set_g3 + COST_G3_LS3);
				if (sel.b7) checkBankElement(src.element0[1][i][j][k], dst.element7[0][src.cCode][i][j], tmp_cost_set_g3 + COST_G3_LS3R);
				if (sel.b8) checkBankElement(src.element0[1][i][j][k], dst.element8[1][src.cCode][i][j], tmp_cost_set_g3);

				// element0[2]: �Ώە����킪 BANK_G0 �ɂ���A�����REGION_GL �ŕ\�����Ă����� (G0->GL, G3->GR, G0=src.cType, G1=i, G2=j, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = sel.b2 = true;
				}
				else if (j == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = sel.b4 = sel.b5 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b7 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b7 = true;
				}

				if (sel.b0) checkBankElement(src.element0[2][i][j][k], dst.element0[2][i][j][k],         tmp_cost_set_g0);
				if (sel.b1) checkBankElement(src.element0[2][i][j][k], dst.element1[1][src.cCode][j][k], tmp_cost_set_g1 + COST_G1_LS1);
				if (sel.b2) checkBankElement(src.element0[2][i][j][k], dst.element2[0][src.cCode][j][k], tmp_cost_set_g1 + COST_G1_LS1R);
				if (sel.b3) checkBankElement(src.element0[2][i][j][k], dst.element3[1][src.cCode][i][k], tmp_cost_set_g2 + COST_G2_LS2);
				if (sel.b4) checkBankElement(src.element0[2][i][j][k], dst.element4[0][src.cCode][i][k], tmp_cost_set_g2 + COST_G2_LS2R);
				if (sel.b5) checkBankElement(src.element0[2][i][j][k], dst.element5[1][src.cCode][i][k], tmp_cost_set_g2);
//				if (sel.b6)
				if (sel.b7) checkBankElement(src.element0[2][i][j][k], dst.element7[0][src.cCode][i][j], tmp_cost_set_g3);
//				if (sel.b8)

				// element1[0]: �Ώە����킪 BANK_G1 �ɂ���A�����REGION_GL �ŕ\�����Ă����� (G1->GL, G2->GR, G0=i, G1=src.cType, G2=j, G3=k)  -------------------------------------------

				tmp_cost_set_g0 = cost_set_g0;
				tmp_cost_set_g1 = cost_set_g1;
				tmp_cost_set_g2 = cost_set_g2;
				tmp_cost_set_g3 = cost_set_g3;

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
					tmp_cost_set_g0 = 0;
				}
				else if (j == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b4 = true;
					tmp_cost_set_g2 = 0;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = sel.b7 = sel.b8 = true;
					tmp_cost_set_g3 = 0;
				}
				else {
					sel.b0 = sel.b1 = sel.b4 = sel.b6 = sel.b7 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element1[0][i][j][k], dst.element0[1][src.cCode][j][k], tmp_cost_set_g0 + COST_G0_LS0);
				if (sel.b1) checkBankElement(src.element1[0][i][j][k], dst.element1[0][i][j][k],         tmp_cost_set_g1);
//				if (sel.b2)
//				if (sel.b3)
				if (sel.b4) checkBankElement(src.element1[0][i][j][k], dst.element4[1][i][src.cCode][k], tmp_cost_set_g2);
//				if (sel.b5)
				if (sel.b6) checkBankElement(src.element1[0][i][j][k], dst.element6[1][i][src.cCode][j], tmp_cost_set_g3 + COST_G3_LS3);
				if (sel.b7) checkBankElement(src.element1[0][i][j][k], dst.element7[1][i][src.cCode][j], tmp_cost_set_g3 + COST_G3_LS3R);
				if (sel.b8) checkBankElement(src.element1[0][i][j][k], dst.element8[2][i][src.cCode][j], tmp_cost_set_g3);

				// element1[1]: �Ώە����킪 BANK_G1 �ɂ���A�����REGION_GL �ŕ\�����Ă����� (G1->GL, G3->GR, G0=i, G1=src.cType, G2=j, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = sel.b4 = sel.b5 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b7 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b3 = sel.b4 = sel.b5 = sel.b7 = true;
				}

				if (sel.b0) checkBankElement(src.element1[1][i][j][k], dst.element0[2][src.cCode][j][k], tmp_cost_set_g0 + COST_G0_LS0);
				if (sel.b1) checkBankElement(src.element1[1][i][j][k], dst.element1[1][i][j][k],         tmp_cost_set_g1);
//				if (sel.b2)
				if (sel.b3) checkBankElement(src.element1[1][i][j][k], dst.element3[1][i][src.cCode][k], tmp_cost_set_g2 + COST_G2_LS2);
				if (sel.b4) checkBankElement(src.element1[1][i][j][k], dst.element4[1][i][src.cCode][k], tmp_cost_set_g2 + COST_G2_LS2R);
				if (sel.b5) checkBankElement(src.element1[1][i][j][k], dst.element5[2][i][src.cCode][k], tmp_cost_set_g2);
//				if (sel.b6)
				if (sel.b7) checkBankElement(src.element1[1][i][j][k], dst.element7[1][i][src.cCode][j], tmp_cost_set_g3);
//				if (sel.b8)

				// element2[0]: �Ώە����킪 BANK_G1 �ɂ���A�����REGION_GR �ŕ\�����Ă����� (G0->GL, G1->GR, G0=i, G1=src.cType, G2=j, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = sel.b4 = sel.b5 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = sel.b7 = sel.b8 = true;
				}
				else {
					sel.b0 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element2[0][i][j][k], dst.element0[0][src.cCode][j][k], tmp_cost_set_g0);
//				if (sel.b1)
				if (sel.b2) checkBankElement(src.element2[0][i][j][k], dst.element2[0][i][j][k],         tmp_cost_set_g1);
				if (sel.b3) checkBankElement(src.element2[0][i][j][k], dst.element3[0][i][src.cCode][k], tmp_cost_set_g2 + COST_G2_LS2);
				if (sel.b4) checkBankElement(src.element2[0][i][j][k], dst.element4[0][i][src.cCode][k], tmp_cost_set_g2 + COST_G2_LS2R);
				if (sel.b5) checkBankElement(src.element2[0][i][j][k], dst.element5[0][i][src.cCode][k], tmp_cost_set_g2);
				if (sel.b6) checkBankElement(src.element2[0][i][j][k], dst.element6[0][i][src.cCode][j], tmp_cost_set_g3 + COST_G3_LS3);
				if (sel.b7) checkBankElement(src.element2[0][i][j][k], dst.element7[0][i][src.cCode][j], tmp_cost_set_g3 + COST_G3_LS3R);
				if (sel.b8) checkBankElement(src.element2[0][i][j][k], dst.element8[0][i][src.cCode][j], tmp_cost_set_g3);

				// element2[1] �Ώە����킪 BANK_G1 �ɂ���A�����REGION_GR �ŕ\�����Ă����� (G2->GL, G1->GR, G0=i, G1=src.cType, G2=j, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = sel.b7 = sel.b8 = true;
				}
				else {
					sel.b0 = sel.b2 = sel.b3 = sel.b6 = sel.b7 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element2[1][i][j][k], dst.element0[0][src.cCode][j][k], tmp_cost_set_g0 + COST_G0_LS0);
//				if (sel.b1)
				if (sel.b2) checkBankElement(src.element2[1][i][j][k], dst.element2[1][i][j][k],         tmp_cost_set_g1);
				if (sel.b3) checkBankElement(src.element2[1][i][j][k], dst.element3[0][i][src.cCode][k], tmp_cost_set_g2);
//				if (sel.b4)
//				if (sel.b5)
				if (sel.b6) checkBankElement(src.element2[1][i][j][k], dst.element6[0][i][src.cCode][j], tmp_cost_set_g3 + COST_G3_LS3);
				if (sel.b7) checkBankElement(src.element2[1][i][j][k], dst.element7[2][i][src.cCode][j], tmp_cost_set_g3 + COST_G3_LS3R);
				if (sel.b8) checkBankElement(src.element2[1][i][j][k], dst.element8[3][i][src.cCode][j], tmp_cost_set_g3);

				// element2[2]: �Ώە����킪 BANK_G1 �ɂ���A�����REGION_GR �ŕ\�����Ă����� (G3->GL, G1->GR, G0=i, G1=src.cType, G2=j, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = sel.b4 = sel.b5 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = true;
				}
				else {
					sel.b0 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = true;
				}

				if (sel.b0) checkBankElement(src.element2[2][i][j][k], dst.element0[0][src.cCode][j][k], tmp_cost_set_g0 + COST_G0_LS0);
//				if (sel.b1)
				if (sel.b2) checkBankElement(src.element2[2][i][j][k], dst.element2[2][i][j][k],         tmp_cost_set_g1);
				if (sel.b3) checkBankElement(src.element2[2][i][j][k], dst.element3[0][i][src.cCode][k], tmp_cost_set_g2 + COST_G2_LS2);
				if (sel.b4) checkBankElement(src.element2[2][i][j][k], dst.element4[2][i][src.cCode][k], tmp_cost_set_g2 + COST_G2_LS2R);
				if (sel.b5) checkBankElement(src.element2[2][i][j][k], dst.element5[3][i][src.cCode][k], tmp_cost_set_g2);
				if (sel.b6) checkBankElement(src.element2[2][i][j][k], dst.element6[0][i][src.cCode][j], tmp_cost_set_g3);
//				if (sel.b7)
//				if (sel.b8)

				// element3[0]: �Ώە����킪 BANK_G2 �ɂ���A�����REGION_GL �ŕ\�����Ă����� (G2->GL, G1->GR, G0=i, G1=j, G2=src.cType, G3=k)  -------------------------------------------

				tmp_cost_set_g0 = cost_set_g0;
				tmp_cost_set_g1 = cost_set_g1;
				tmp_cost_set_g2 = cost_set_g2;
				tmp_cost_set_g3 = cost_set_g3;

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
					tmp_cost_set_g0 = 0;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b2 = true;
					tmp_cost_set_g1 = 0;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = sel.b7 = sel.b8 = true;
					tmp_cost_set_g3 = 0;
				}
				else {
					sel.b0 = sel.b2 = sel.b3 = sel.b6 = sel.b7 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element3[0][i][j][k], dst.element0[0][j][src.cCode][k], tmp_cost_set_g0 + COST_G0_LS0);
//				if (sel.b1)
				if (sel.b2) checkBankElement(src.element3[0][i][j][k], dst.element2[1][i][src.cCode][k], tmp_cost_set_g1);
				if (sel.b3) checkBankElement(src.element3[0][i][j][k], dst.element3[0][i][j][k],         tmp_cost_set_g2);
//				if (sel.b4)
//				if (sel.b5)
				if (sel.b6) checkBankElement(src.element3[0][i][j][k], dst.element6[0][i][j][src.cCode], tmp_cost_set_g3 + COST_G3_LS3);
				if (sel.b7) checkBankElement(src.element3[0][i][j][k], dst.element7[2][i][j][src.cCode], tmp_cost_set_g3 + COST_G3_LS3R);
				if (sel.b8) checkBankElement(src.element3[0][i][j][k], dst.element8[3][i][j][src.cCode], tmp_cost_set_g3);

				// element3[1]: �Ώە����킪 BANK_G2 �ɂ���A�����REGION_GL �ŕ\�����Ă����� (G2->GL, G3->GR, G0=i, G1=j, G2=src.cType, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b7 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b7 = true;
				}

				if (sel.b0) checkBankElement(src.element3[1][i][j][k], dst.element0[2][j][src.cCode][k], tmp_cost_set_g0 + COST_G0_LS0);
				if (sel.b1)	checkBankElement(src.element3[1][i][j][k], dst.element1[1][i][src.cCode][k], tmp_cost_set_g1 + COST_G1_LS1);
				if (sel.b2) checkBankElement(src.element3[1][i][j][k], dst.element2[1][i][src.cCode][k], tmp_cost_set_g1 + COST_G1_LS1R);
				if (sel.b3) checkBankElement(src.element3[1][i][j][k], dst.element3[1][i][j][k],         tmp_cost_set_g2);
//				if (sel.b4)
//				if (sel.b5)
//				if (sel.b6)
				if (sel.b7) checkBankElement(src.element3[1][i][j][k], dst.element7[2][i][j][src.cCode], tmp_cost_set_g3);
//				if (sel.b8)

				// element4[0]: �Ώە����킪 BANK_G2 �ɂ���A�����REGION_GR �ŕ\�����Ă����� (G0->GL, G2->GR, G0=i, G1=j, G2=src.cType, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = sel.b7 = sel.b8 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b2 = sel.b4 = sel.b6 = sel.b7 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element4[0][i][j][k], dst.element0[1][j][src.cCode][k], tmp_cost_set_g0);
				if (sel.b1)	checkBankElement(src.element4[0][i][j][k], dst.element1[0][i][src.cCode][k], tmp_cost_set_g1 + COST_G1_LS1);
				if (sel.b2) checkBankElement(src.element4[0][i][j][k], dst.element2[0][i][src.cCode][k], tmp_cost_set_g1 + COST_G1_LS1R);
//				if (sel.b3)
				if (sel.b4) checkBankElement(src.element4[0][i][j][k], dst.element4[0][i][j][k],         tmp_cost_set_g2);
//				if (sel.b5)
				if (sel.b6) checkBankElement(src.element4[0][i][j][k], dst.element6[1][i][j][src.cCode], tmp_cost_set_g3 + COST_G3_LS3);
				if (sel.b7) checkBankElement(src.element4[0][i][j][k], dst.element7[0][i][j][src.cCode], tmp_cost_set_g3 + COST_G3_LS3R);
				if (sel.b8) checkBankElement(src.element4[0][i][j][k], dst.element8[1][i][j][src.cCode], tmp_cost_set_g3);

				// element4[1]: �Ώە����킪 BANK_G2 �ɂ���A�����REGION_GR �ŕ\�����Ă����� (G1->GL, G2->GR, G0=i, G1=j, G2=src.cType, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = sel.b7 = sel.b8 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b4 = sel.b6 = sel.b7 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element4[1][i][j][k], dst.element0[1][j][src.cCode][k], tmp_cost_set_g0 + COST_G0_LS0);
				if (sel.b1)	checkBankElement(src.element4[1][i][j][k], dst.element1[0][i][src.cCode][k], tmp_cost_set_g1);
//				if (sel.b2)
//				if (sel.b3)
				if (sel.b4) checkBankElement(src.element4[1][i][j][k], dst.element4[1][i][j][k],         tmp_cost_set_g2);
//				if (sel.b5)
				if (sel.b6) checkBankElement(src.element4[1][i][j][k], dst.element6[1][i][j][src.cCode], tmp_cost_set_g3 + COST_G3_LS3);
				if (sel.b7) checkBankElement(src.element4[1][i][j][k], dst.element7[1][i][j][src.cCode], tmp_cost_set_g3 + COST_G3_LS3R);
				if (sel.b8) checkBankElement(src.element4[1][i][j][k], dst.element8[2][i][j][src.cCode], tmp_cost_set_g3);

				// element4[2]: �Ώە����킪 BANK_G2 �ɂ���A�����REGION_GR �ŕ\�����Ă����� (G3->GL, G2->GR, G0=i, G1=j, G2=src.cType, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b2 =  sel.b4 = sel.b6 = true;
				}

				if (sel.b0) checkBankElement(src.element4[2][i][j][k], dst.element0[1][j][src.cCode][k], tmp_cost_set_g0 + COST_G0_LS0);
				if (sel.b1)	checkBankElement(src.element4[2][i][j][k], dst.element1[0][i][src.cCode][k], tmp_cost_set_g1 + COST_G1_LS1);
				if (sel.b2) checkBankElement(src.element4[2][i][j][k], dst.element2[2][i][src.cCode][k], tmp_cost_set_g1 + COST_G1_LS1R);
//				if (sel.b3)
				if (sel.b4) checkBankElement(src.element4[2][i][j][k], dst.element4[2][i][j][k],         tmp_cost_set_g2);
//				if (sel.b5)
				if (sel.b6) checkBankElement(src.element4[2][i][j][k], dst.element6[1][i][j][src.cCode], tmp_cost_set_g3);
//				if (sel.b7)
//				if (sel.b8)

				// element5[0]: �Ώە����킪 BANK_G2 �ɂ���A������V���O���V�t�g (SS2) �ŕ\�����Ă����� (G0->GL, G1->GR, G0=i, G1=j, G2=src.cType, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = sel.b7 = sel.b8 = true;
				}
				else {
					sel.b0 = sel.b2 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element5[0][i][j][k], dst.element0[0][j][src.cCode][k], tmp_cost_set_g0);
//				if (sel.b1)
				if (sel.b2) checkBankElement(src.element5[0][i][j][k], dst.element2[0][i][src.cCode][k], tmp_cost_set_g1);
//				if (sel.b3)
//				if (sel.b4)
				if (sel.b5) checkBankElement(src.element5[0][i][j][k], dst.element5[0][i][j][k],         tmp_cost_set_g2);
				if (sel.b6) checkBankElement(src.element5[0][i][j][k], dst.element6[0][i][j][src.cCode], tmp_cost_set_g3 + COST_G3_LS3);
				if (sel.b7) checkBankElement(src.element5[0][i][j][k], dst.element7[0][i][j][src.cCode], tmp_cost_set_g3 + COST_G3_LS3R);
				if (sel.b8) checkBankElement(src.element5[0][i][j][k], dst.element8[0][i][j][src.cCode], tmp_cost_set_g3);

				// element5[1]: �Ώە����킪 BANK_G2 �ɂ���A������V���O���V�t�g (SS2) �ŕ\�����Ă����� (G0->GL, G3->GR, G0=i, G1=j, G2=src.cType, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b7 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b2 = sel.b5 = sel.b7 = true;
				}

				if (sel.b0) checkBankElement(src.element5[1][i][j][k], dst.element0[2][j][src.cCode][k], tmp_cost_set_g0);
				if (sel.b1)	checkBankElement(src.element5[1][i][j][k], dst.element1[1][i][src.cCode][k], tmp_cost_set_g1 + COST_G1_LS1);
				if (sel.b2) checkBankElement(src.element5[1][i][j][k], dst.element2[0][i][src.cCode][k], tmp_cost_set_g1 + COST_G1_LS1R);
//				if (sel.b3)
//				if (sel.b4)
				if (sel.b5) checkBankElement(src.element5[1][i][j][k], dst.element5[1][i][j][k],         tmp_cost_set_g2);
//				if (sel.b6)
				if (sel.b7) checkBankElement(src.element5[1][i][j][k], dst.element7[0][i][j][src.cCode], tmp_cost_set_g3);
//				if (sel.b8)

				// element5[2]: �Ώە����킪 BANK_G2 �ɂ���A������V���O���V�t�g (SS2) �ŕ\�����Ă����� (G1->GL, G3->GR, G0=i, G1=j, G2=src.cType, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b7 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b5 = sel.b7 = true;
				}

				if (sel.b0) checkBankElement(src.element5[2][i][j][k], dst.element0[2][j][src.cCode][k], tmp_cost_set_g0 + COST_G0_LS0);
				if (sel.b1)	checkBankElement(src.element5[2][i][j][k], dst.element1[1][i][src.cCode][k], tmp_cost_set_g1);
//				if (sel.b2)
//				if (sel.b3)
//				if (sel.b4)
				if (sel.b5) checkBankElement(src.element5[2][i][j][k], dst.element5[2][i][j][k],         tmp_cost_set_g2);
//				if (sel.b6)
				if (sel.b7) checkBankElement(src.element5[2][i][j][k], dst.element7[1][i][j][src.cCode], tmp_cost_set_g3);
//				if (sel.b8)

				// element5[3]: �Ώە����킪 BANK_G2 �ɂ���A������V���O���V�t�g (SS2) �ŕ\�����Ă����� (G3->GL, G1->GR, G0=i, G1=j, G2=src.cType, G3=k)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G3 �� dst.cType �̏ꍇ
					sel.b6 = true;
				}
				else {
					sel.b0 = sel.b2 = sel.b5 = sel.b6 = true;
				}

				if (sel.b0) checkBankElement(src.element5[3][i][j][k], dst.element0[0][j][src.cCode][k], tmp_cost_set_g0 + COST_G0_LS0);
//				if (sel.b1)
				if (sel.b2) checkBankElement(src.element5[3][i][j][k], dst.element2[2][i][src.cCode][k], tmp_cost_set_g1);
//				if (sel.b3)
//				if (sel.b4)
				if (sel.b5) checkBankElement(src.element5[3][i][j][k], dst.element5[3][i][j][k],         tmp_cost_set_g2);
				if (sel.b6) checkBankElement(src.element5[3][i][j][k], dst.element6[0][i][j][src.cCode], tmp_cost_set_g3);
//				if (sel.b7)
//				if (sel.b8)

				// element6[0]: �Ώە����킪 BANK_G3 �ɂ���A�����REGION_GL �ŕ\�����Ă����� (G3->GL, G1->GR, G0=i, G1=j, G2=k, G3=src.cType)  -------------------------------------------

				tmp_cost_set_g0 = cost_set_g0;
				tmp_cost_set_g1 = cost_set_g1;
				tmp_cost_set_g2 = cost_set_g2;
				tmp_cost_set_g3 = cost_set_g3;

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
					tmp_cost_set_g0 = 0;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b2 = true;
					tmp_cost_set_g1 = 0;
				}
				else if (k == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = sel.b4 = sel.b5 = true;
					tmp_cost_set_g2 = 0;
				}
				else {
					sel.b0 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = true;
				}

				if (sel.b0) checkBankElement(src.element6[0][i][j][k], dst.element0[0][j][k][src.cCode], tmp_cost_set_g0 + COST_G0_LS0);
//				if (sel.b1)
				if (sel.b2) checkBankElement(src.element6[0][i][j][k], dst.element2[2][i][k][src.cCode], tmp_cost_set_g1);
				if (sel.b3) checkBankElement(src.element6[0][i][j][k], dst.element3[0][i][j][src.cCode], tmp_cost_set_g2 + COST_G2_LS2);
				if (sel.b4) checkBankElement(src.element6[0][i][j][k], dst.element4[2][i][j][src.cCode], tmp_cost_set_g2 + COST_G2_LS2R);
				if (sel.b5) checkBankElement(src.element6[0][i][j][k], dst.element5[3][i][j][src.cCode], tmp_cost_set_g2);
				if (sel.b6) checkBankElement(src.element6[0][i][j][k], dst.element6[0][i][j][k],         tmp_cost_set_g3);
//				if (sel.b7)
//				if (sel.b8)

				// element6[1]: �Ώە����킪 BANK_G3 �ɂ���A�����REGION_GL �ŕ\�����Ă����� (G3->GL, G2->GR, G0=i, G1=j, G2=k, G3=src.cType)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b4 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b2 = sel.b4 = sel.b6 = true;
				}

				if (sel.b0) checkBankElement(src.element6[1][i][j][k], dst.element0[1][j][k][src.cCode], tmp_cost_set_g0 + COST_G0_LS0);
				if (sel.b1) checkBankElement(src.element6[1][i][j][k], dst.element1[0][i][k][src.cCode], tmp_cost_set_g1 + COST_G1_LS1);
				if (sel.b2) checkBankElement(src.element6[1][i][j][k], dst.element2[2][i][k][src.cCode], tmp_cost_set_g1 + COST_G1_LS1R);
//				if (sel.b3)
				if (sel.b4) checkBankElement(src.element6[1][i][j][k], dst.element4[2][i][j][src.cCode], tmp_cost_set_g2);
//				if (sel.b5)
				if (sel.b6) checkBankElement(src.element6[1][i][j][k], dst.element6[1][i][j][k],         tmp_cost_set_g3);
//				if (sel.b7)
//				if (sel.b8)

				// element7[0]: �Ώە����킪 BANK_G3 �ɂ���A�����REGION_GR �ŕ\�����Ă����� (G0->GL, G3->GR, G0=i, G1=j, G2=k, G3=src.cType)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = sel.b4 = sel.b5 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b7 = true;
				}

				if (sel.b0) checkBankElement(src.element7[0][i][j][k], dst.element0[2][j][k][src.cCode], tmp_cost_set_g0);
				if (sel.b1) checkBankElement(src.element7[0][i][j][k], dst.element1[1][i][k][src.cCode], tmp_cost_set_g1 + COST_G1_LS1);
				if (sel.b2) checkBankElement(src.element7[0][i][j][k], dst.element2[0][i][k][src.cCode], tmp_cost_set_g1 + COST_G1_LS1R);
				if (sel.b3) checkBankElement(src.element7[0][i][j][k], dst.element3[1][i][j][src.cCode], tmp_cost_set_g2 + COST_G2_LS2);
				if (sel.b4) checkBankElement(src.element7[0][i][j][k], dst.element4[0][i][j][src.cCode], tmp_cost_set_g2 + COST_G2_LS2R);
				if (sel.b5) checkBankElement(src.element7[0][i][j][k], dst.element5[1][i][j][src.cCode], tmp_cost_set_g2);
//				if (sel.b6)
				if (sel.b7) checkBankElement(src.element7[0][i][j][k], dst.element7[0][i][j][k],         tmp_cost_set_g3);
//				if (sel.b8)

				// element7[1]: �Ώە����킪 BANK_G3 �ɂ���A�����REGION_GR �ŕ\�����Ă����� (G1->GL, G3->GR, G0=i, G1=j, G2=k, G3=src.cType)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = true;
				}
				else if (k == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = sel.b4 = sel.b5 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b3 = sel.b4 = sel.b5 = sel.b7 = true;
				}

				if (sel.b0) checkBankElement(src.element7[1][i][j][k], dst.element0[2][j][k][src.cCode], tmp_cost_set_g0 + COST_G0_LS0);
				if (sel.b1) checkBankElement(src.element7[1][i][j][k], dst.element1[1][i][k][src.cCode], tmp_cost_set_g1);
//				if (sel.b2)
				if (sel.b3) checkBankElement(src.element7[1][i][j][k], dst.element3[1][i][j][src.cCode], tmp_cost_set_g2 + COST_G2_LS2);
				if (sel.b4) checkBankElement(src.element7[1][i][j][k], dst.element4[1][i][j][src.cCode], tmp_cost_set_g2 + COST_G2_LS2R);
				if (sel.b5) checkBankElement(src.element7[1][i][j][k], dst.element5[2][i][j][src.cCode], tmp_cost_set_g2);
//				if (sel.b6)
				if (sel.b7) checkBankElement(src.element7[1][i][j][k], dst.element7[1][i][j][k],         tmp_cost_set_g3);
//				if (sel.b8)

				// element7[2]: �Ώە����킪 BANK_G3 �ɂ���A�����REGION_GR �ŕ\�����Ă����� (G2->GL, G3->GR, G0=i, G1=j, G2=k, G3=src.cType)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b7 = true;
				}

				if (sel.b0) checkBankElement(src.element7[2][i][j][k], dst.element0[2][j][k][src.cCode], tmp_cost_set_g0 + COST_G0_LS0);
				if (sel.b1) checkBankElement(src.element7[2][i][j][k], dst.element1[1][i][k][src.cCode], tmp_cost_set_g1 + COST_G1_LS1);
				if (sel.b2) checkBankElement(src.element7[2][i][j][k], dst.element2[1][i][k][src.cCode], tmp_cost_set_g1 + COST_G1_LS1R);
				if (sel.b3) checkBankElement(src.element7[2][i][j][k], dst.element3[1][i][j][src.cCode], tmp_cost_set_g2);
//				if (sel.b4)
//				if (sel.b5)
//				if (sel.b6)
				if (sel.b7) checkBankElement(src.element7[2][i][j][k], dst.element7[2][i][j][k],         tmp_cost_set_g3);
//				if (sel.b8)

				// element8[0]: �Ώە����킪 BANK_G3 �ɂ���A������V���O���V�t�g (SS3) �ŕ\�����Ă����� (G0->GL, G1->GR, G0=i, G1=j, G2=k, G3=src.cType)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = sel.b4 = sel.b5 = true;
				}
				else {
					sel.b0 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element8[0][i][j][k], dst.element0[0][j][k][src.cCode], tmp_cost_set_g0);
//				if (sel.b1)
				if (sel.b2) checkBankElement(src.element8[0][i][j][k], dst.element2[0][i][k][src.cCode], tmp_cost_set_g1);
				if (sel.b3) checkBankElement(src.element8[0][i][j][k], dst.element3[0][i][j][src.cCode], tmp_cost_set_g2 + COST_G2_LS2);
				if (sel.b4) checkBankElement(src.element8[0][i][j][k], dst.element4[0][i][j][src.cCode], tmp_cost_set_g2 + COST_G2_LS2R);
				if (sel.b5) checkBankElement(src.element8[0][i][j][k], dst.element5[0][i][j][src.cCode], tmp_cost_set_g2);
//				if (sel.b6)
//				if (sel.b7)
				if (sel.b8) checkBankElement(src.element8[0][i][j][k], dst.element8[0][i][j][k],         tmp_cost_set_g3);

				// element8[1]: �Ώە����킪 BANK_G3 �ɂ���A������V���O���V�t�g (SS3) �ŕ\�����Ă����� (G0->GL, G2->GR, G0=i, G1=j, G2=k, G3=src.cType)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b4 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b2 = sel.b4 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element8[1][i][j][k], dst.element0[1][j][k][src.cCode], tmp_cost_set_g0);
				if (sel.b1) checkBankElement(src.element8[1][i][j][k], dst.element1[0][i][k][src.cCode], tmp_cost_set_g1 + COST_G1_LS1);
				if (sel.b2) checkBankElement(src.element8[1][i][j][k], dst.element2[0][i][k][src.cCode], tmp_cost_set_g1 + COST_G1_LS1R);
//				if (sel.b3)
				if (sel.b4) checkBankElement(src.element8[1][i][j][k], dst.element4[0][i][j][src.cCode], tmp_cost_set_g2);
//				if (sel.b5)
//				if (sel.b6)
//				if (sel.b7)
				if (sel.b8) checkBankElement(src.element8[1][i][j][k], dst.element8[1][i][j][k],         tmp_cost_set_g3);

				// element8[2]: �Ώە����킪 BANK_G3 �ɂ���A������V���O���V�t�g (SS3) �ŕ\�����Ă����� (G1->GL, G2->GR, G0=i, G1=j, G2=k, G3=src.cType)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b1 = true;
				}
				else if (k == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b4 = true;
				}
				else {
					sel.b0 = sel.b1 = sel.b4 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element8[2][i][j][k], dst.element0[1][j][k][src.cCode], tmp_cost_set_g0 + COST_G0_LS0);
				if (sel.b1) checkBankElement(src.element8[2][i][j][k], dst.element1[0][i][k][src.cCode], tmp_cost_set_g1);
//				if (sel.b2)
//				if (sel.b3)
				if (sel.b4) checkBankElement(src.element8[2][i][j][k], dst.element4[1][i][j][src.cCode], tmp_cost_set_g2);
//				if (sel.b5)
//				if (sel.b6)
//				if (sel.b7)
				if (sel.b8) checkBankElement(src.element8[2][i][j][k], dst.element8[2][i][j][k],         tmp_cost_set_g3);

				// element8[3]: �Ώە����킪 BANK_G3 �ɂ���A������V���O���V�t�g (SS3) �ŕ\�����Ă����� (G2->GL, G1->GR, G0=i, G1=j, G2=k, G3=src.cType)  -------------------------------------------

				sel.b0 = sel.b1 = sel.b2 = sel.b3 = sel.b4 = sel.b5 = sel.b6 = sel.b7 = sel.b8 = false;

				if (i == dst.cCode) {									// BANK_G0 �� dst.cType �̏ꍇ
					sel.b0 = true;
				}
				else if (j == dst.cCode) {								// BANK_G1 �� dst.cType �̏ꍇ
					sel.b2 = true;
				}
				else if (k == dst.cCode) {								// BANK_G2 �� dst.cType �̏ꍇ
					sel.b3 = true;
				}
				else {
					sel.b0 = sel.b2 = sel.b3 = sel.b8 = true;
				}

				if (sel.b0) checkBankElement(src.element8[3][i][j][k], dst.element0[0][j][k][src.cCode], tmp_cost_set_g0 + COST_G0_LS0);
//				if (sel.b1)
				if (sel.b2) checkBankElement(src.element8[3][i][j][k], dst.element2[1][i][k][src.cCode], tmp_cost_set_g1);
				if (sel.b3) checkBankElement(src.element8[3][i][j][k], dst.element3[0][i][j][src.cCode], tmp_cost_set_g2);
//				if (sel.b4)
//				if (sel.b5)
//				if (sel.b6)
//				if (sel.b7)
				if (sel.b8) checkBankElement(src.element8[3][i][j][k], dst.element8[3][i][j][k],         tmp_cost_set_g3);

				// �����܂� -----------------------------------------------------------------------------------------------------------------------------------------------------------------
			}
		}
	}

	return;
}


void calcBankUnitCost(BankUnit &src, BankUnit &dst)
{
	if (dst.cType == F_NULL) return;

	int32_t		numSrc = 1;
	int32_t		numDst = 1;

	if ((src.cType == F_HIRAGANA) || (src.cType == F_KATAKANA)) {
		numSrc = 2;
	}
	else if (src.cType == F_KANACOMMON) {
		numSrc = 3;
	}

	if ((dst.cType == F_HIRAGANA) || (dst.cType == F_KATAKANA)) {
		numDst = 2;
	}
	else if (dst.cType == F_KANACOMMON) {
		numDst = 3;
	}

	for (int32_t i = 0; i < numSrc; i++)
	{
		for (int32_t j = 0; j < numDst; j++)
		{
			calcBankGroupCost(src.group[i], dst.group[j]);
		}

		processBankGroup(src.group[i]);
	}

	return;
}


BankSet* checkMojiSequenceUTF16(const char16_t *sbuf, const size_t total_length, const ConvStatus *status)
{
	//
	// �\�[�X UTF-16 �����񒆂̕�����̕��я��A���𐔂��A�ŒZ��8�P�ʕ���������ƂȂ�悤�Ƀo���N�����蓖�Ă�D
	//

	uint16_t		*mojiClass	= new uint16_t[total_length + 2];
	size_t			*mojiNum	= new size_t[total_length + 2];
	size_t			mojiLen		= 0;
	int32_t			numCType	= 4;

	countMojiSequenceUTF16(sbuf, total_length, status, mojiClass, mojiNum, &mojiLen, &numCType);									// �\�[�X������̕����̕��я����擾����. 

	// ����

	BankUnit	*src = new BankUnit;
	BankUnit	*dst = new BankUnit;
	BankUnit	*tmp;

	initBankElementSeq(*src, numCType, mojiLen);
	initBankElementSeq(*dst, numCType, mojiLen);
	
	initBankUnit(src, F_NULL, 0);

	// ���C��

	size_t	count = mojiLen - 2;

	while (1)
	{
		tmp = src;
		src = dst;
		dst = tmp;

		initBankUnit(src, mojiClass[count], mojiNum[count]);

		calcBankUnitCost(*src, *dst);

		if (count == 0) break;
		count--;
	}

	// ���ʂ̒��o

	const int32_t gL = status->region[REGION_GL];
	const int32_t gR = status->region[REGION_GR];
	const int32_t g0 = getCTypeCode(status->bank[BANK_G0]);
	const int32_t g1 = getCTypeCode(status->bank[BANK_G1]);
	const int32_t g2 = getCTypeCode(status->bank[BANK_G2]);
	const int32_t g3 = getCTypeCode(status->bank[BANK_G3]);

	BankElement *result = NULL;

	if ((gL == BANK_G0) && (gR == BANK_G1)) result = &src->group[0].element0[0][g1][g2][g3];
	if ((gL == BANK_G0) && (gR == BANK_G2)) result = &src->group[0].element0[1][g1][g2][g3];
	if ((gL == BANK_G0) && (gR == BANK_G3)) result = &src->group[0].element0[2][g1][g2][g3];
	if ((gL == BANK_G1) && (gR == BANK_G2)) result = &src->group[0].element1[0][g0][g2][g3];
	if ((gL == BANK_G1) && (gR == BANK_G3)) result = &src->group[0].element1[1][g0][g2][g3];
	if ((gL == BANK_G2) && (gR == BANK_G1)) result = &src->group[0].element3[0][g0][g1][g3];
	if ((gL == BANK_G2) && (gR == BANK_G3)) result = &src->group[0].element3[1][g0][g1][g3];
	if ((gL == BANK_G3) && (gR == BANK_G1)) result = &src->group[0].element6[0][g0][g1][g2];
	if ((gL == BANK_G3) && (gR == BANK_G2)) result = &src->group[0].element6[1][g0][g1][g2];

	BankSet	*bankSeq = new BankSet[result->blen + 1];													// ���ʂ�ۑ�����o�b�t�@���m�ۂ���D��ŌĂяo�����ŊJ������K�v����D

	for (size_t i = 0; i < result->blen; i++) {
		bankSeq[i] = result->bseq[result->blen - i - 1];												// ���ʂ��t���ɃR�s�[���Ď��[����
	}

	bankSeq[result->blen] = USE_BANK_NONE;																// �f�[�^�̏I�[

	// ��n��

	delete [] mojiClass;
	delete [] mojiNum;

	deleteBankElementSeq(*src);
	deleteBankElementSeq(*dst);

	delete src;
	delete dst;

	return bankSeq;
}


BankSet* checkMojiSequenceUTF8(const uint8_t *sbuf, const size_t total_length, const ConvStatus *status)
{
	//
	// �\�[�X UTF-8 �����񒆂̕�����̕��я��A���𐔂��A�ŒZ��8�P�ʕ���������ƂȂ�悤�Ƀo���N�����蓖�Ă�D
	//

	uint16_t		*mojiClass = new uint16_t[total_length + 2];
	size_t			*mojiNum = new size_t[total_length + 2];
	size_t			mojiLen = 0;
	int32_t			numCType = 4;

	countMojiSequenceUTF8(sbuf, total_length, status, mojiClass, mojiNum, &mojiLen, &numCType);			// �\�[�X������̕����̕��я����擾����. 

	// ����

	BankUnit	*src = new BankUnit;
	BankUnit	*dst = new BankUnit;
	BankUnit	*tmp;

	initBankElementSeq(*src, numCType, mojiLen);
	initBankElementSeq(*dst, numCType, mojiLen);

	initBankUnit(src, F_NULL, 0);

	// ���C��

	size_t	count = mojiLen - 2;

	while (1)
	{
		tmp = src;
		src = dst;
		dst = tmp;

		initBankUnit(src, mojiClass[count], mojiNum[count]);

		calcBankUnitCost(*src, *dst);

		if (count == 0) break;
		count--;
	}

	// ���ʂ̒��o

	const int32_t gL = status->region[REGION_GL];
	const int32_t gR = status->region[REGION_GR];
	const int32_t g0 = getCTypeCode(status->bank[BANK_G0]);
	const int32_t g1 = getCTypeCode(status->bank[BANK_G1]);
	const int32_t g2 = getCTypeCode(status->bank[BANK_G2]);
	const int32_t g3 = getCTypeCode(status->bank[BANK_G3]);

	BankElement *result = NULL;

	if ((gL == BANK_G0) && (gR == BANK_G1)) result = &src->group[0].element0[0][g1][g2][g3];
	if ((gL == BANK_G0) && (gR == BANK_G2)) result = &src->group[0].element0[1][g1][g2][g3];
	if ((gL == BANK_G0) && (gR == BANK_G3)) result = &src->group[0].element0[2][g1][g2][g3];
	if ((gL == BANK_G1) && (gR == BANK_G2)) result = &src->group[0].element1[0][g0][g2][g3];
	if ((gL == BANK_G1) && (gR == BANK_G3)) result = &src->group[0].element1[1][g0][g2][g3];
	if ((gL == BANK_G2) && (gR == BANK_G1)) result = &src->group[0].element3[0][g0][g1][g3];
	if ((gL == BANK_G2) && (gR == BANK_G3)) result = &src->group[0].element3[1][g0][g1][g3];
	if ((gL == BANK_G3) && (gR == BANK_G1)) result = &src->group[0].element6[0][g0][g1][g2];
	if ((gL == BANK_G3) && (gR == BANK_G2)) result = &src->group[0].element6[1][g0][g1][g2];

	BankSet	*bankSeq = new BankSet[result->blen + 1];													// ���ʂ�ۑ�����o�b�t�@���m�ۂ���D��ŌĂяo�����ŊJ������K�v����D

	for (size_t i = 0; i < result->blen; i++) {
		bankSeq[i] = result->bseq[result->blen - i - 1];												// ���ʂ��t���ɃR�s�[���Ď��[����
	}

	bankSeq[result->blen] = USE_BANK_NONE;																// �f�[�^�̏I�[

																										// ��n��

	delete[] mojiClass;
	delete[] mojiNum;

	deleteBankElementSeq(*src);
	deleteBankElementSeq(*dst);

	delete src;
	delete dst;

	return bankSeq;
}


BankSet* checkMojiSequenceU32T(const uint32_t *sbuf, const size_t total_length, const ConvStatus *status)
{
	//
	// �\�[�X U32T �����񒆂̕�����̕��я��A���𐔂��A�ŒZ��8�P�ʕ���������ƂȂ�悤�Ƀo���N�����蓖�Ă�D
	//

	uint16_t		*mojiClass = new uint16_t[total_length + 2];
	size_t			*mojiNum = new size_t[total_length + 2];
	size_t			mojiLen = 0;
	int32_t			numCType = 4;

	countMojiSequenceU32T(sbuf, total_length, status, mojiClass, mojiNum, &mojiLen, &numCType);				// �\�[�X������̕����̕��я����擾����. 

	// ����

	BankUnit	*src = new BankUnit;
	BankUnit	*dst = new BankUnit;
	BankUnit	*tmp;

	initBankElementSeq(*src, numCType, mojiLen);
	initBankElementSeq(*dst, numCType, mojiLen);

	initBankUnit(src, F_NULL, 0);

	// ���C��

	size_t	count = mojiLen - 2;

	while (1)
	{
		tmp = src;
		src = dst;
		dst = tmp;

		initBankUnit(src, mojiClass[count], mojiNum[count]);

		calcBankUnitCost(*src, *dst);

		if (count == 0) break;
		count--;
	}

	// ���ʂ̒��o

	const int32_t gL = status->region[REGION_GL];
	const int32_t gR = status->region[REGION_GR];
	const int32_t g0 = getCTypeCode(status->bank[BANK_G0]);
	const int32_t g1 = getCTypeCode(status->bank[BANK_G1]);
	const int32_t g2 = getCTypeCode(status->bank[BANK_G2]);
	const int32_t g3 = getCTypeCode(status->bank[BANK_G3]);

	BankElement *result = NULL;

	if ((gL == BANK_G0) && (gR == BANK_G1)) result = &src->group[0].element0[0][g1][g2][g3];
	if ((gL == BANK_G0) && (gR == BANK_G2)) result = &src->group[0].element0[1][g1][g2][g3];
	if ((gL == BANK_G0) && (gR == BANK_G3)) result = &src->group[0].element0[2][g1][g2][g3];
	if ((gL == BANK_G1) && (gR == BANK_G2)) result = &src->group[0].element1[0][g0][g2][g3];
	if ((gL == BANK_G1) && (gR == BANK_G3)) result = &src->group[0].element1[1][g0][g2][g3];
	if ((gL == BANK_G2) && (gR == BANK_G1)) result = &src->group[0].element3[0][g0][g1][g3];
	if ((gL == BANK_G2) && (gR == BANK_G3)) result = &src->group[0].element3[1][g0][g1][g3];
	if ((gL == BANK_G3) && (gR == BANK_G1)) result = &src->group[0].element6[0][g0][g1][g2];
	if ((gL == BANK_G3) && (gR == BANK_G2)) result = &src->group[0].element6[1][g0][g1][g2];

	BankSet	*bankSeq = new BankSet[result->blen + 1];													// ���ʂ�ۑ�����o�b�t�@���m�ۂ���D��ŌĂяo�����ŊJ������K�v����D

	for (size_t i = 0; i < result->blen; i++) {
		bankSeq[i] = result->bseq[result->blen - i - 1];												// ���ʂ��t���ɃR�s�[���Ď��[����
	}

	bankSeq[result->blen] = USE_BANK_NONE;																// �f�[�^�̏I�[

																										// ��n��

	delete[] mojiClass;
	delete[] mojiNum;

	deleteBankElementSeq(*src);
	deleteBankElementSeq(*dst);

	delete src;
	delete dst;

	return bankSeq;
}


