#include "../common/comm.h"
#include "../common/file.h"

#define RISE_CONTINUOUS_DAYS        (3)
#define RISE_WATCH_DAYS             (1)

VOID STAT_Rise(IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstBeginData, IN FILE_WHOLE_DATA_S *pstFirstData)
{
    ULONG i;
    BOOL_T bIsCont;
    FLOAT fPrevRise, fWatchRise, fWatchDrop;
    FILE_WHOLE_DATA_S *pstPrev = pstBeginData-1;
    FILE_WHOLE_DATA_S *pstBase = pstPrev-RISE_CONTINUOUS_DAYS;
    FILE_WHOLE_DATA_S *pstWatch = pstBeginData;

    // make sure array is not out of bound
    if ((pstBeginData-pstFirstData) < RISE_CONTINUOUS_DAYS) {
        pstBase = pstFirstData;
        pstPrev = pstBase+RISE_CONTINUOUS_DAYS;
        pstWatch = pstPrev+1;
        ulEntryCnt -= pstWatch-pstBeginData;
    }

    for (i=0;i<ulEntryCnt;i++, pstBase++, pstPrev++, pstWatch++) {
        //if (INVAILD_ULONG != pstBase->ulRsv) continue;
        
        bIsCont = GetTotalRise(RISE_CONTINUOUS_DAYS, pstPrev, RISE_TYPE_END, &fPrevRise);
        if (BOOL_FALSE == bIsCont) continue;

        GetTotalRise(RISE_WATCH_DAYS, pstWatch, RISE_TYPE_HIGH, &fWatchRise);
        GetTotalRise(RISE_WATCH_DAYS, pstWatch, RISE_TYPE_LOW, &fWatchDrop);

        printf("%d,%f,%f,%f\n", pstWatch->ulDate, fPrevRise, fWatchRise, fWatchDrop);
    }

    return;
}

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
            STAT_Rise(ulStatCnt, pstStatData, astWholeData);
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
