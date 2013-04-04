#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

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

#define FILE_FLAG_THS       0x8
#define FILE_FLAG_MIN5      0x5
#define FILE_FLAG_DAY       0x24
#define FILE_FLAG_WGT       0x55
#define FILE_FLAG_ALL_R     0xAA
#define FILE_FLAG_ALL_W     0xAB
#define FILE_VAILD_FACTOR   0x88888888
#define FILE_VAILD_PRICE    0xCCCCCCCC

#define FILE_NAME_LEN       256

#define DATE_BREAKDOWN(ulDate, ulYear, ulMon, ulDay)    \
{                                       \
    (ulYear)  = (ulDate) / 10000;       \
    (ulDate) -= (ulYear) * 10000;       \
    (ulMon)   = (ulDate) / 100;         \
    (ulDate) -= (ulMon)  * 100;         \
    (ulDay)   = (ulDate);               \
}

#define DATE_ASSEMBLE(ulYear, ulMon, ulDay)             \
    (ULONG)(((ulYear)*10000)+((ulMon)*100)+(ulDay));

VOID DebugOutString(IN const CHAR *szFmt, ...);
ULONG GetIndexByDate(IN ULONG ulDate, IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstWholeEntry);

