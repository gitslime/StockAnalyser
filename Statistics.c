#include "common/comm.h"
#include "common/file.h"
#include "method/method.h"

#define STAT_WATCH_SHORT        (3)
#define STAT_WATCH_MIDDLE       (10)
#define STAT_WATCH_LONG         (30)

VOID STAT_PrintWatch(IN ULONG ulIndex, IN ULONG ulEntryCnt, IN ULONG ulWatchDays, IN FILE_WHOLE_DATA_S *pstCurr)
{
    FLOAT fWatchRise;

    if ((ulIndex+ulWatchDays)>=ulEntryCnt) {
        printf("-100,-100,");        //print an invaild number for excel which deals only numbers
    }
    else {
        (VOID)GetTotalRise(ulWatchDays, pstCurr+ulWatchDays, RISE_TYPE_LOW, &fWatchRise);
        printf("%f,", fWatchRise);
        (VOID)GetTotalRise(ulWatchDays, pstCurr+ulWatchDays, RISE_TYPE_HIGH, &fWatchRise);
        printf("%f,", fWatchRise);
    }
    return;
}

VOID STAT_Distribute(IN ULONG ulCode, IN CHAR *szDir, IN ULONG ulMethod, IN ULONG ulBeginDate, IN ULONG ulEndDate)
{
    ULONG i, ulEntryCnt;
    ULONG ulBeginIndex, ulEndIndex;
    //ULONG ulThreshPrice;
    METHOD_FUNC_SET_S stMethodFunc;
    Choose_PF pfChoose = NULL;
    Statistics_PF pfStat = NULL;
    CHOOSE_PRE_DEAL_S stDealInfo;
    FILE_WHOLE_DATA_S *astWholeData = NULL;
    FILE_WHOLE_DATA_S *pstStatData = NULL;

    ulEntryCnt = FILE_GetFileData(ulCode, szDir, FILE_TYPE_CUSTOM, (VOID**)&astWholeData);
    if (0 == ulEntryCnt) return;

    ulBeginIndex = GetIndexByDate(ulBeginDate, INDEX_NEXT, ulEntryCnt, astWholeData);
    ulEndIndex   = GetIndexByDate(ulEndDate,   INDEX_PREV, ulEntryCnt, astWholeData);
    if (ulBeginIndex>ulEndIndex) {
        DebugOutString("%lu: invaild begin date: %lu, end date: %lu\n", ulCode, ulBeginDate, ulEndDate);
        free(astWholeData);
        return;
    }

    METHOD_GetFuncSet(ulMethod, &stMethodFunc);    // get function by method
    pfChoose = stMethodFunc.pfDailyChoose;
    pfStat = stMethodFunc.pfStatistics;
    memset(&stDealInfo, 0, sizeof(stDealInfo));
    for (i=ulBeginIndex,pstStatData = &astWholeData[ulBeginIndex];i<=ulEndIndex;i++, pstStatData++) {
        #if 0
        ulThreshPrice=FILE_REAL2PRICE(stDealInfo.fThresholdPrice);
        if (((BOOL_FALSE == stDealInfo.bIsHigher) && (pstStatData->stDailyPrice.ulLow < ulThreshPrice)) ||
            ((BOOL_FALSE != stDealInfo.bIsHigher) && (pstStatData->stDailyPrice.ulEnd > ulThreshPrice)))
        {
            printf("%d,",pstStatData->ulDate);
            pfStat(pstStatData);
            STAT_PrintWatch(i, ulEntryCnt, STAT_WATCH_SHORT, pstStatData);        
            STAT_PrintWatch(i, ulEntryCnt, STAT_WATCH_MIDDLE, pstStatData);
            STAT_PrintWatch(i, ulEntryCnt, STAT_WATCH_LONG, pstStatData);
            printf("\n");
            memset(&stDealInfo, 0, sizeof(stDealInfo));
        }
        
        (VOID)pfChoose(i, pstStatData, &stDealInfo);
        #endif
        pfStat(i, pstStatData);
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
