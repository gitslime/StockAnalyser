#include "comm.h"
#include <stdarg.h>
#include <time.h>

BOOL_T g_bIsDebugMode = BOOL_FALSE;

#define DEBUG_BUF_SIZE      (1024)

VOID DebugOutString(IN const CHAR *szFmt, ...)
{
    va_list pArg;  
    CHAR szBuf[DEBUG_BUF_SIZE];

    if (BOOL_FALSE == g_bIsDebugMode) 
        return;

    va_start(pArg,szFmt);
    vsprintf_s(szBuf, DEBUG_BUF_SIZE, szFmt, pArg);
    va_end(pArg);

    printf("%s", szBuf);

    return;
}

//not use recursion because of overhead of function call
ULONG GetIndexByDate(IN ULONG ulDate, IN ULONG ulFlag, IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstWholeEntry)
{
    int iBegin, iEnd, iMid;         //flag maybe nagative
    FILE_WHOLE_DATA_S *pstMidEntry = pstWholeEntry;    

    iBegin=0;
    iEnd=ulEntryCnt-1;
    while (1) {
        iMid=(iBegin+iEnd)>>1;           //get middle point (>>1)=(/2)
        pstMidEntry = pstWholeEntry + iMid;

        if (ulDate == pstMidEntry->ulDate) break;

        if (ulDate < pstMidEntry->ulDate) {
            iEnd = iMid-1;
        }
        else {
            iBegin = iMid+1;
        }

        if (iBegin>iEnd) {
            switch (ulFlag) {
                case INDEX_EXACT:
                    DebugOutString("%d not in the list\n",ulDate);
                    iMid = INVAILD_ULONG;
                    break;
                case INDEX_PREV:
                    iMid = iEnd;
                    break;
                case INDEX_NEXT:
                    iMid = iBegin;
                    break;
                default:
                    assert(0);
                    return INVAILD_ULONG;
            }
            break;
        }
    }

    return ((ULONG)iMid >= ulEntryCnt) ? INVAILD_ULONG : (ULONG)iMid;
}

ULONG GetCurrentDate(VOID)
{
    time_t RawTime; 
    struct tm LocalTime;
    
    time(&RawTime); 
    localtime_s (&LocalTime, &RawTime);

    return DATE_ASSEMBLE(LocalTime.tm_year, LocalTime.tm_mon, LocalTime.tm_mday);
}

ULONG GetMethod(IN CHAR* szMethod)
{
    ULONG ulMethod;
    
    if (0 == _stricmp(szMethod, "rise")) {
        ulMethod = METHOD_RISE;
    }
    else {
        printf("invaild method type\n");
        ulMethod = INVAILD_ULONG;
        exit(2);
    }
    return ulMethod;
}


ULONG GetCodeList(IN CHAR* szCode, OUT ULONG **ppulList) 
{
    ULONG i;
    ULONG ulCode;
    ULONG ulCodeCnt;
    
    if (0 == _stricmp(szCode, "all")) {
        ulCodeCnt = g_ulTotalCount;
        *ppulList = g_aulStockCode;
    }
    else {
        ulCode = (ULONG)atol(szCode);
        for (i=0;i<g_ulTotalCount;i++) {
            if (ulCode == g_aulStockCode[i]) break;
        }

        if (g_ulTotalCount == i) {
            ulCodeCnt = 0;      // not found
            DebugOutString("code %06u not found!\n", ulCode);
        }
        else {
            ulCodeCnt = 1;
            *ppulList = &g_aulStockCode[i];
        }
    }
    
    return ulCodeCnt;
}

// get total rise of n days, including current
BOOL_T GetTotalRise(IN ULONG ulCnt, IN FILE_WHOLE_DATA_S *pstCurrent, IN ULONG ulType, OUT FLOAT *pfTotalRise)
{
    ULONG i;
    BOOL_T bIsContinuous = BOOL_TRUE;
    ULONG ulBaseEndPrice;
    ULONG ulHigh, ulLow;
    FLOAT fPrevPrice, fCurrPrice;
    FLOAT fMulti, fAdder;
    FILE_WHOLE_DATA_S *pstBase = pstCurrent - ulCnt;
    FILE_WHOLE_DATA_S *pstTemp = pstBase+1;

    assert(FILE_VAILD_PRICE == pstBase->stDailyPrice.ulFlag);
    ulBaseEndPrice = pstBase->stDailyPrice.ulEnd;

    fPrevPrice = (FLOAT)ulBaseEndPrice;
    fMulti = 1;
    fAdder = 0;
    ulHigh = pstTemp->stDailyPrice.ulHigh;
    ulLow  = pstTemp->stDailyPrice.ulLow;
    for (i=0;i<ulCnt;i++) {
        if (FILE_VAILD_FACTOR == pstTemp->stFactor.ulFlag) {
            fMulti *= pstTemp->stFactor.fMulti;
            fAdder += pstTemp->stFactor.fAdder;
        }
        
        if (RISE_TYPE_END == ulType) {
            fCurrPrice = fMulti * pstTemp->stDailyPrice.ulEnd + fAdder;
        }
        else if (RISE_TYPE_HIGH == ulType) {
            ulHigh = MAX(pstTemp->stDailyPrice.ulHigh, ulHigh);
            fCurrPrice = fMulti * ulHigh + fAdder;
        }
        else if (RISE_TYPE_LOW == ulType) {
            ulLow = MIN(pstTemp->stDailyPrice.ulHigh, ulLow);
            fCurrPrice = fMulti * ulLow + fAdder;
        }
        else if (RISE_TYPE_BEGIN == ulType) {
            fCurrPrice = fMulti * pstTemp->stDailyPrice.ulBegin + fAdder;
        }
        else {
            assert(0);
        }
        if (fCurrPrice <= 1.02*fPrevPrice) bIsContinuous = BOOL_FALSE;

        pstTemp++;
        fPrevPrice = fCurrPrice;
    }

    *pfTotalRise = fCurrPrice / ulBaseEndPrice - 1;

    return bIsContinuous;
}

VOID RandomInit(VOID)
{
    srand((unsigned int)time(0));
    return;
}

// return a random unsigned long in [ulMin, ulMax)
ULONG RandomUlong(IN ULONG ulMin, IN ULONG ulMax)
{
    if (ulMin == ulMax) return ulMin;
    
    assert(ulMax > ulMin);
    return (rand() % (ulMax - ulMin)) + ulMin;
}

// return a random float in [fMin, fMax]
FLOAT RandomFloat(IN FLOAT fMin, IN FLOAT fMax)
{
    assert(fMax >= fMin);
    return (FLOAT)(rand()/(double)(RAND_MAX))*(fMax-fMin)+fMin;
}

BOOL_T IsVaildDate(IN ULONG ulDate)
{
    ULONG ulYear, ulMon, ulDay;
    ULONG ulDateEachMon[12]={31,29,31,30,31,30,31,31,30,31,30,31};
    
    DATE_BREAKDOWN(ulDate,ulYear,ulMon,ulDay);

    if ((ulYear < 1990) || (ulYear > 2050)) return BOOL_FALSE;
    if ((ulMon < 1) || (ulMon > 12)) return BOOL_FALSE;
    if ((ulDay < 1) || (ulDay > ulDateEachMon[ulMon])) return BOOL_FALSE;

    return BOOL_TRUE;
}

VOID GetFactor(IN ULONG ulStartDate, IN FILE_WHOLE_DATA_S *pstCurrData,
               OUT FLOAT *pfMulti, OUT FLOAT *pfAdder)
{
    FLOAT fMulti, fAdder;
    FILE_WHOLE_DATA_S *pstTemp = pstCurrData;

    fMulti = 1;
    fAdder = 0;
    while (1) {
        if (pstTemp->ulDate <= ulStartDate) break;
        
        if (FILE_VAILD_FACTOR == pstTemp->stFactor.ulFlag) {
            fMulti *= pstTemp->stFactor.fMulti;
            fAdder += pstTemp->stFactor.fAdder;
        }
		pstTemp--;
    }

    *pfMulti = fMulti;
    *pfAdder = fAdder;
    
    return;
}

