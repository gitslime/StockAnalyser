#include "../common/comm.h"
#include "../common/file.h"

#define RISE_CONTINUOUS_DAYS        (3)
#define RISE_WATCH_DAYS             (2)

VOID STATIC_Rise(IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstWholeData)
{
    ULONG i;
    BOOL_T bIsCont;
    FLOAT fPrevRise, fWatchRise, fWatchDrop;
    FILE_WHOLE_DATA_S *pstCurr = pstWholeData+RISE_CONTINUOUS_DAYS+1;
    FILE_WHOLE_DATA_S *pstWatch = pstCurr+RISE_WATCH_DAYS;

    ulEntryCnt -= RISE_WATCH_DAYS;
    for (i=RISE_CONTINUOUS_DAYS+1;i<ulEntryCnt;i++, pstCurr++, pstWatch++) {
        bIsCont = GetTotalRise(RISE_CONTINUOUS_DAYS, pstCurr, RISE_TYPE_END, &fPrevRise);
        if (BOOL_FALSE == bIsCont) continue;

        GetTotalRise(RISE_WATCH_DAYS, pstWatch, RISE_TYPE_HIGH, &fWatchRise);
        GetTotalRise(RISE_WATCH_DAYS, pstWatch, RISE_TYPE_LOW, &fWatchDrop);

        printf("%d,%f,%f,%f\n", pstCurr->ulDate, fPrevRise, fWatchRise, fWatchDrop);
    }

    return;
}

VOID STATIC_Distribute(IN ULONG ulCode, IN CHAR *szDir, IN ULONG ulMethod)
{
    ULONG ulEntryCnt;
    FILE_WHOLE_DATA_S *astWholeData = NULL;

    ulEntryCnt = FILE_GetFileData(ulCode, szDir, FILE_TYPE_CUSTOM, &astWholeData);
    if (0 == ulEntryCnt) return;

    switch (ulMethod) {
        case METHOD_RISE:
            STATIC_Rise(ulEntryCnt, astWholeData);
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
    ULONG ulStockCode;
    ULONG ulMethod;
    ULONG *pulCodeList = NULL;
    
    //check parameter
    if ((argc < 4) || (argc > 5)) {
        printf("USAGE: %s method path { code | all } [debug]", argv[0]);
        exit(1);
    }

    if (0 == _stricmp(argv[argc-1], "debug"))
        g_bIsDebugMode = BOOL_TRUE;
    
    ulMethod  = GetMethod(argv[1]);
    ulCodeCnt = GetCodeList(argv[3], &pulCodeList);

    for (i=0;i<ulCodeCnt;i++) {
        STATIC_Distribute(pulCodeList[i], argv[2], ulMethod);
    }

    return 0;
}
