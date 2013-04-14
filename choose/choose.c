#include "../common/comm.h"
#include "../common/file.h"

typedef struct tagPreDealInfo
{
    BOOL_T bIsSell;     //true:sell; false:buy
    USHORT usDealHour;
    USHORT usDealMin;
    BOOL_T bIsHigher;   //true:higher; false:lower
    FLOAT  fThresholdPrice;
}CHOOSE_PRE_DEAL_S;

#define CHOOSE_RISE_DAYS    (2)
#define CHOOSE_RISE_THRESHOLD_RATE      (1.1F)
#define CHOOSE_RISE_BUY_HOUR    (14)
#define CHOOSE_RISE_BUY_MIN     (50)

BOOL_T CHOOSE_Rise(IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    BOOL_T bIsContinuous;
    FLOAT  fPrevRise;
    FILE_WHOLE_DATA_S *pstBase = pstCurrData - CHOOSE_RISE_DAYS;
    
    bIsContinuous = GetTotalRise(CHOOSE_RISE_DAYS, (pstCurrData+1), RISE_TYPE_END, &fPrevRise);
    if (BOOL_FALSE == bIsContinuous) return BOOL_FALSE;

    pstDealInfo->bIsSell    = BOOL_FALSE;
    pstDealInfo->usDealHour = CHOOSE_RISE_BUY_HOUR;
    pstDealInfo->usDealMin  = CHOOSE_RISE_BUY_MIN;
    pstDealInfo->bIsHigher  = BOOL_TRUE;
    pstDealInfo->fThresholdPrice = FILE_PRICE2REAL(CHOOSE_RISE_THRESHOLD_RATE * pstBase->stDailyPrice.ulEnd);

    return BOOL_TRUE;
}

VOID CHOOSE_Distribute(IN ULONG ulCode, IN ULONG ulDate, IN CHAR * szDir, IN ULONG ulMethod)
{
    BOOL_T bIsDeal;
    CHOOSE_PRE_DEAL_S stDealInfo;
    ULONG ulEntryCnt;
    ULONG ulIndex;
    FILE_WHOLE_DATA_S *astWholeData = NULL;
    
    ulEntryCnt = FILE_GetFileData(ulCode, szDir, FILE_TYPE_CUSTOM, &astWholeData);
    if (0 == ulEntryCnt) return;

    ulIndex = GetIndexByDate(ulDate, ulEntryCnt, astWholeData);
    if (INVAILD_ULONG == ulIndex) {
        free(astWholeData);
        DebugOutString("invalid date=%u, code=%06u\n", ulDate, ulCode);
        return;
    }

    switch (ulMethod) {
        case METHOD_RISE:
            bIsDeal = CHOOSE_Rise(&astWholeData[ulIndex], &stDealInfo);
            break;
        default:
            printf("method not support\n");
    }

    free(astWholeData);

    if (BOOL_TRUE == bIsDeal) {
        printf("%06u,%u,%u,%u,%u,%.2f\n",
               ulCode, stDealInfo.bIsSell, stDealInfo.usDealHour, stDealInfo.usDealMin, 
               stDealInfo.bIsHigher, stDealInfo.fThresholdPrice);
    }
    else {
        DebugOutString("%06u not in deal\n",ulCode);
    }

    return;
}

int main(int argc,char *argv[]) 
{
    ULONG i, ulCodeCnt;
    ULONG ulDate;
    ULONG ulMethod;
    ULONG *pulCodeList = NULL;
    
    //check parameter
    if ((argc < 5) || (argc > 6)) {
        printf("USAGE: %s method path date { code | all } [debug]", argv[0]);
        exit(1);
    }

    if (0 == _stricmp(argv[argc-1], "debug"))
        g_bIsDebugMode = BOOL_TRUE;
    
    ulMethod = GetMethod(argv[1]);
    ulDate = (ULONG)atol(argv[3]);
    ulCodeCnt = GetCodeList(argv[4], &pulCodeList);

    for (i=0;i<ulCodeCnt;i++) {
        CHOOSE_Distribute(pulCodeList[i], ulDate, argv[2], ulMethod);
    }
    
    return 0;
}

