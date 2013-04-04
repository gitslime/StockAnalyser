#include <time.h>
#include "comm.h"

ULONG GetFileType(IN CHAR* szType)
{
    ULONG ulFileType;
    
    if (0 == _stricmp(szType, "ths")) {
        ulFileType = FILE_FLAG_THS;
    }
    else if (0 == _stricmp(szType, "custom")) {
        ulFileType = FILE_FLAG_ALL_R;
    }
    else if (0 == _stricmp(szType, "weight")) {
        ulFileType = FILE_FLAG_WGT;
    }
    else {
        ulFileType = INVAILD_ULONG;
    }
    return ulFileType;
}

ULONG GetCurrentDate(VOID)
{
    time_t RawTime; 
    struct tm *LocalTime;
    
    time(&RawTime); 
    LocalTime = localtime_r (&RawTime);

    return DATE_ASSEMBLE(LocalTime->tm_year, LocalTime->tm_mon, LocalTime->tm_mday);
}

BOOL_T CheckDate(IN CHAR *szDate)
{
    ULONG ulDate = (ULONG)atol(szDate);
    ULONG ulYear, ulMon, ulDay;

    DATE_BREAKDOWN(ulDate,ulYear,ulMon,ulDay);

    if ((ulYear < 1990) || ()
    return;
}

int main(int argc,char *argv[])
{
    ULONG ulStockCode;
    
    //check parameter
    if ((argc < 5) || (argc > 5)) {
        printf("USAGE: %s code date path type [debug]", argv[0]);
        exit(1);
    }
    ulStockCode = (ULONG)atol(argv[1]);

    if (0 == _stricmp(argv[argc], "debug"))
        g_bIsDebugMode = BOOL_TRUE;

    return 0;
}
