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
#define INDEX_CODE_SH           (900001)
#define INDEX_CODE_SZ           (900002)
#define INDEX_CODE_ZXB          (900003)
#define INDEX_CODE_CYB          (900004)
ULONG GetCodeByIndex(IN CHAR *szIndex);

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

typedef struct tagStockData
{
    ULONG ulStockCode;
    ULONG ulEntryCnt;
    BOOL_T bIsHold;     // for get wish list to skip hold stocks
    FILE_WHOLE_DATA_S *pstCurrData;
    FILE_WHOLE_DATA_S *astWholeData;
}SIM_STOCK_DATA_S;

typedef struct tagPreDealInfo
{
    BOOL_T bIsSell;     //true:sell; false:buy
    USHORT usDealHour;
    USHORT usDealMin;
    BOOL_T bIsHigher;   //true:higher; false:lower
    FLOAT  fThresholdPrice;
}CHOOSE_PRE_DEAL_S;

typedef struct tagStockCtrl
{
    //ULONG  ulWishPrice;
    ULONG  ulGainPrice;
    ULONG  ulLossPrice;
    ULONG  ulBuyDate;
    ULONG  ulBuyPrice;
    ULONG  ulSellDate;
    ULONG  ulSellPrice;
}STOCK_CTRL_S;

#define FILE_TYPE_THS           0x10000000
#define FILE_TYPE_THS_MIN5      0x10000005
#define FILE_TYPE_THS_DAY       0x10000024
#define FILE_TYPE_QL            0x20000000
#define FILE_TYPE_QL_WGT        0x20008000
#define FILE_TYPE_CUSTOM        0xF0000000

#define FILE_VAILD_FACTOR   0x88888888
#define FILE_VAILD_PRICE    0xCCCCCCCC

#define FILE_PRICE2REAL(FilePrice)     (((FLOAT)(FilePrice))/(1000))
#define FILE_REAL2PRICE(FilePrice)     ((ULONG)((FilePrice)*(1000)))

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

#define INDEX_EXACT     (0x00)      //the same date exactly
#define INDEX_PREV      (0x10)      //if not found, return the nearest date before ulDate
#define INDEX_NEXT      (0x11)      //if not found, return the nearest date after ulDate
ULONG GetIndexByDate(IN ULONG ulDate, IN ULONG ulFlag, IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstWholeEntry);

ULONG GetCodeList(IN CHAR* szCode, OUT ULONG **ppulList);

#define RISE_TYPE_BEGIN             (0x01UL)
#define RISE_TYPE_HIGH              (0x02UL)
#define RISE_TYPE_LOW               (0x03UL)
#define RISE_TYPE_END               (0x04UL)
#define STOCK_RISE_THREASHOLD       (0.099F)
#define GET_RATE(Check, Base)       (((FLOAT)(Check))/((FLOAT)(Base))-1)

BOOL_T GetTotalRise(IN ULONG ulCnt, IN FILE_WHOLE_DATA_S *pstCurrent, IN ULONG ulType, OUT FLOAT *pfTotalRise);
FLOAT GetVolRatio(IN FILE_WHOLE_DATA_S *pstData);

VOID RandomInit(VOID);
ULONG RandomUlong(IN ULONG ulMin, IN ULONG ulMax);
FLOAT RandomFloat(IN FLOAT fMin, IN FLOAT fMax);

BOOL_T IsVaildDate(IN ULONG ulDate);
VOID GetFactor(IN ULONG ulStartDate, IN FILE_WHOLE_DATA_S *pstCurrData, OUT FLOAT *pfMulti, OUT FLOAT *pfAdder);
ULONG GetMean(IN ULONG ulDays, IN FILE_WHOLE_DATA_S *pstData, IN ULONG ulPrevMean);
ULONG GetDateInterval(IN ULONG ulStartDate, IN ULONG ulEndDate);
ULONG GetBuyCnt(IN ULONG ulHandLimit, IN ULONG ulBuyPrice);

#define CANDLE_TYPE_LINE                (1UL)
#define CANDLE_TYPE_T_INVERSE           (2UL)
#define CANDLE_TYPE_T_RIGHT             (3UL)
#define CANDLE_TYPE_HAMMER_INVERSE      (4UL)
#define CANDLE_TYPE_HAMMER_RIGHT        (5UL)
#define CANDLE_TYPE_CROSS_LONG          (6UL)
#define CANDLE_TYPE_CROSS               (7UL)
#define CANDLE_TYPE_LITTLE_LOSS         (8UL)
#define CANDLE_TYPE_LITTLE_GAIN         (9UL)
#define CANDLE_TYPE_MIDDLE_LOSS         (10UL)
#define CANDLE_TYPE_MIDDLE_GAIN         (11UL)
#define CANDLE_TYPE_BARE_FEET_LOSS      (12UL)
#define CANDLE_TYPE_BARE_FEET_GAIN      (13UL)
#define CANDLE_TYPE_BARE_HEAD_LOSS      (14UL)
#define CANDLE_TYPE_BARE_HEAD_GAIN      (15UL)
#define CANDLE_TYPE_BARE_LARGE_LOSS     (16UL)
#define CANDLE_TYPE_BARE_LARGE_GAIN     (17UL)
#define CANDLE_TYPE_LARGE_LOSS          (18UL)
#define CANDLE_TYPE_LARGE_GAIN          (19UL)
ULONG GetCandleType(IN FILE_WHOLE_DATA_S *pstData);

// single link list
typedef struct tagSllNode{
    VOID *pNext;
}SLL_NODE_S;

VOID SLL_InsertInTail(IN SLL_NODE_S *pstHead, IN SLL_NODE_S *pstNode);
VOID SLL_DeleteNode(IN SLL_NODE_S *pstHead, IN SLL_NODE_S *pstNode);
VOID SLL_FreeAll(IN SLL_NODE_S *pstHead);

#endif

