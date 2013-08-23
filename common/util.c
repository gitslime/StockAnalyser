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

ULONG GetIndexByCode(IN ULONG ulCode, IN ULONG ulEntryCnt, IN STOCK_CTRL_S *pstStockCtrl)
{
    int iBegin, iEnd, iMid;         //flag maybe nagative
    STOCK_CTRL_S *pstMidEntry = pstStockCtrl;    

    iBegin=0;
    iEnd=ulEntryCnt-1;
    while (1) {
        iMid=(iBegin+iEnd)>>1;           //get middle point (>>1)=(/2)
        pstMidEntry = pstStockCtrl + iMid;

        if (ulCode == pstMidEntry->ulCode) break;

        if (ulCode < pstMidEntry->ulCode) {
            iEnd = iMid-1;
        }
        else {
            iBegin = iMid+1;
        }

        if (iBegin>iEnd) {
            iMid = INVAILD_ULONG;
            break;
        }
    }

    return (ULONG)iMid;
}


ULONG GetCurrentDate(VOID)
{
    time_t RawTime; 
    struct tm LocalTime;
    
    time(&RawTime); 
    localtime_s (&LocalTime, &RawTime);

    return DATE_ASSEMBLE(LocalTime.tm_year, LocalTime.tm_mon, LocalTime.tm_mday);
}

ULONG GetDayto1904(IN ULONG ulDate)
{
#define PARAM_DAYS_OF_4YEAR     (365*4+1)

    ULONG ulYear, ulMon, ulDay;
    ULONG ulYearCnt, ulMonCnt, ulDayCnt, i;
    CHAR aucDaysofMon[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,   
                           31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,   
                           31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,   
                           31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    assert(ulDate > 19040101);

    DATE_BREAKDOWN(ulDate, ulYear, ulMon, ulDay);

    ulDayCnt  = 0;
    ulYearCnt = ulYear - 1904;
    ulDayCnt += (ulYearCnt/4)*PARAM_DAYS_OF_4YEAR;
    ulMonCnt  = (ulYearCnt%4)*12 + ulMon - 1;
    assert(ulMonCnt < 48);
    for(i=0; i < ulMonCnt; i++)
        ulDayCnt += aucDaysofMon[i];
    ulDayCnt += ulDay - 1;

    return ulDayCnt;
}

ULONG GetWeekDay(IN ULONG ulDate)
{
    // 19040101 is friday, skip week end
    return ((GetDayto1904(ulDate)+4)%7)+1;
}

ULONG GetDateInterval(IN ULONG ulStartDate, IN ULONG ulEndDate)
{
    assert(ulStartDate < ulEndDate);

    return GetDayto1904(ulEndDate) - GetDayto1904(ulStartDate);
}

BOOL_T IsVaildDate(IN ULONG ulDate)
{
    ULONG ulYear, ulMon, ulDay;
    ULONG ulCheckDate=ulDate;
    ULONG ulDateEachMon[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};
    
    DATE_BREAKDOWN(ulDate,ulYear,ulMon,ulDay);

    if ((ulYear < 1990) || (ulYear > 2050)) return BOOL_FALSE;
    if ((ulMon < 1) || (ulMon > 12)) return BOOL_FALSE;
    if ((2==ulMon) && (29==ulDay) && (0==(ulYear&0x03))) return BOOL_TRUE;  // check leap year
    if ((ulDay < 1) || (ulDay > ulDateEachMon[ulMon])) return BOOL_FALSE;
    
    return BOOL_TRUE;
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






