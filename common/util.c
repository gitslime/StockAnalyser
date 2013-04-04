#include "comm.h"
#include <stdarg.h>

BOOL_T g_bIsDebugMode = BOOL_FALSE;

BOOL_T IsVaildStock(IN ULONG ulCode)
{
    if (((600000 <= ulCode) && (ulCode < 602000)) ||
        ((603000 <= ulCode) && (ulCode < 604000)) ||
        ((     1 <= ulCode) && (ulCode <   3000)) ||   
        ((300001 <= ulCode) && (ulCode < 300500)))
        return BOOL_TRUE;
    else
        return BOOL_FALSE;
}

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
ULONG GetIndexByDate(IN ULONG ulDate, IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstWholeEntry)
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
            DebugOutString("%d not in the list\n",ulDate);
            return INVAILD_ULONG;
        }
    }

    return (ULONG)iMid;
}

