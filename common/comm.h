#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#ifndef __COMM_H__
#define __COMM_H__

/* type begin */
typedef void VOID;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef unsigned char UCHAR;
typedef char CHAR;
typedef long LONG;
typedef float FLOAT;
typedef short SHORT;
typedef USHORT BOOL_T;
#define BOOL_TRUE       (1)
#define BOOL_FALSE      (0)

#define IN
#define OUT
#define INOUT

#define INVAILD_ULONG   (0xFFFFFFFF)
/* type end */

extern BOOL_T g_bIsDebugMode;
VOID DebugOutString(IN const CHAR *szFmt, ...);

#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))

#define STOCK_TOTAL_CNT          (2500)

typedef struct tagPrice
{
    ULONG  ulBegin;
    ULONG  ulHigh;
    ULONG  ulLow;
    ULONG  ulEnd;
    ULONG  ulVol;
    ULONG  ulFlag;
}PRICE_S;

typedef struct tagFactor
{
    ULONG ulFlag;
    FLOAT fMulti;
    FLOAT fAdder;
}FACTOR_S;

typedef struct tagWholeData
{
    CHAR  szComment[16];
    ULONG ulDate;
    ULONG ulRsv;
    PRICE_S stDailyPrice;
    PRICE_S astMin30Price[8];
    FACTOR_S stFactor;
    ULONG  ulPad;
}FILE_WHOLE_DATA_S;

#define FILE_TYPE_THS           0x10000000
#define FILE_TYPE_THS_MIN5      0x10000005
#define FILE_TYPE_THS_DAY       0x10000024
#define FILE_TYPE_QL            0x20000000
#define FILE_TYPE_QL_WGT        0x20008000
#define FILE_TYPE_CUSTOM        0xF0000000

#define FILE_VAILD_FACTOR   0x88888888
#define FILE_VAILD_PRICE    0xCCCCCCCC



#define DATE_BREAKDOWN(ulDate, ulYear, ulMon, ulDay)    \
{                                       \
    (ulYear)  = (ulDate) / 10000;       \
    (ulDate) -= (ulYear) * 10000;       \
    (ulMon)   = (ulDate) / 100;         \
    (ulDate) -= (ulMon)  * 100;         \
    (ulDay)   = (ulDate);               \
}

#define DATE_ASSEMBLE(ulYear, ulMon, ulDay)             \
    ((ULONG)(((ulYear)*10000)+((ulMon)*100)+(ulDay)))

ULONG GetCurrentDate(VOID);

ULONG GetIndexByDate(IN ULONG ulDate, IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstWholeEntry);
ULONG FILE_GetFileData(IN ULONG ulStockCode, IN CHAR *szDir, IN ULONG ulFileType, OUT VOID **ppFileData);

#endif

