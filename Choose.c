#include "common/comm.h"
#include "common/file.h"
#include "method/method.h"

VOID CHOOSE_Distribute(IN ULONG ulCode, IN ULONG ulDate, IN CHAR * szDir, IN ULONG ulMethod)
{
    BOOL_T bIsDeal;
    CHOOSE_PRE_DEAL_S stDealInfo;
    ULONG ulEntryCnt;
    ULONG ulIndex;
    FILE_WHOLE_DATA_S *astWholeData = NULL;
    
    ulEntryCnt = FILE_GetFileData(ulCode, szDir, FILE_TYPE_CUSTOM, &astWholeData);
    if (0 == ulEntryCnt) return;

    ulIndex = GetIndexByDate(ulDate, INDEX_EXACT, ulEntryCnt, astWholeData);
    if (INVAILD_ULONG == ulIndex) {
        free(astWholeData);
        DebugOutString("invalid date=%u, code=%06u\n", ulDate, ulCode);
        return;
    }

    switch (ulMethod) {
        case METHOD_RISE:
            bIsDeal = RISE_Choose(ulIndex, &astWholeData[ulIndex], &stDealInfo);
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
    RandomInit();

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

