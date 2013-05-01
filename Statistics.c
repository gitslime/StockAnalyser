#include "common/comm.h"
#include "common/file.h"
#include "method/method.h"

VOID STAT_Distribute(IN ULONG ulCode, IN CHAR *szDir, IN ULONG ulMethod, IN ULONG ulBeginDate, IN ULONG ulEndDate)
{
    ULONG ulEntryCnt, ulStatCnt;
    ULONG ulBeginIndex, ulEndIndex;
    FILE_WHOLE_DATA_S *astWholeData = NULL;
    FILE_WHOLE_DATA_S *pstStatData = NULL;

    ulEntryCnt = FILE_GetFileData(ulCode, szDir, FILE_TYPE_CUSTOM, &astWholeData);
    if (0 == ulEntryCnt) return;

    ulBeginIndex = GetIndexByDate(ulBeginDate, INDEX_NEXT, ulEntryCnt, astWholeData);
    ulEndIndex   = GetIndexByDate(ulEndDate,   INDEX_PREV, ulEntryCnt, astWholeData);
    if (ulBeginIndex>ulEndIndex) {
        DebugOutString("%u: invaild begin date: %u, end date: %u\n", ulCode, ulBeginDate, ulEndDate);
        free(astWholeData);
        return;
    }
    ulStatCnt   = ulEndIndex - ulBeginIndex + 1;
    pstStatData = &astWholeData[ulBeginIndex];
    assert(ulStatCnt <= ulEntryCnt);

    switch (ulMethod) {
        case METHOD_RISE:
            RISE_Statistics(ulStatCnt, pstStatData, astWholeData);
            break;
        default:
            printf("method not support\n");
    }

    free(astWholeData);

    return;
}

int main(int argc,char *argv[]) 
{
    ULONG i, ulCodeCnt;
    ULONG ulMethod;
    ULONG ulBeginDate, ulEndDate;
    ULONG *pulCodeList = NULL;
    
    //check parameter
    if ((argc < 6) || (argc > 7)) {
        printf("USAGE: %s method path begin-date end-date { code | all } [debug]", argv[0]);
        exit(1);
    }

    if (0 == _stricmp(argv[argc-1], "debug"))
        g_bIsDebugMode = BOOL_TRUE;
    
    ulMethod  = GetMethod(argv[1]);
    ulCodeCnt = GetCodeList(argv[5], &pulCodeList);
    ulBeginDate = (ULONG)atol(argv[3]);
    ulEndDate = (ULONG)atol(argv[4]);

    for (i=0;i<ulCodeCnt;i++) {
        STAT_Distribute(pulCodeList[i], argv[2], ulMethod, ulBeginDate, ulEndDate);
    }

    return 0;
}
