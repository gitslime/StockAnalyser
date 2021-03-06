#include "common/comm.h"
#include "common/file.h"
#include "method/method.h"

VOID CHOOSE_Track(IN ULONG ulCode, IN ULONG ulDate, IN CHAR * szDir, IN ULONG ulMethod)
{
    ULONG ulEntryCnt;
    ULONG ulIndex;
    ULONG ulLatestEndPrice;
    ULONG ulGainPriceDiff, ulLossPriceDiff;
    METHOD_FUNC_SET_S stMethodFunc;
    STOCK_CTRL_S stStockCtrl;
    FILE_WHOLE_DATA_S *astWholeData = NULL;
    FILE_WHOLE_DATA_S *pstLatestData = NULL;
    
    ulEntryCnt = FILE_GetFileData(ulCode, szDir, FILE_TYPE_CUSTOM, (VOID**)&astWholeData);
    if (0 == ulEntryCnt) return;

    ulIndex = GetIndexByDate(ulDate, INDEX_EXACT, ulEntryCnt, astWholeData);
    if (INVAILD_ULONG == ulIndex) {
        free(astWholeData);
        DebugOutString("invalid date=%lu, code=%06lu\n", ulDate, ulCode);
        return;
    }
    pstLatestData=&astWholeData[ulEntryCnt-1];
    ulLatestEndPrice = pstLatestData->stDailyPrice.ulEnd;

    // get function by method
    METHOD_GetFuncSet(ulMethod, &stMethodFunc);
    stMethodFunc.pfSetParam(0, NULL);

    // fill stock ctrl
    memset(&stStockCtrl, 0, sizeof(stStockCtrl));
    stStockCtrl.ulBuyDate = ulDate;
    stStockCtrl.ulGainPrice = stMethodFunc.pfGetGainPrice(pstLatestData, &stStockCtrl);
    stStockCtrl.ulLossPrice = stMethodFunc.pfGetLossPrice(pstLatestData, &stStockCtrl);

    free(astWholeData);

    // price may be out of lastest end price
    ulGainPriceDiff=(stStockCtrl.ulGainPrice > (ulLatestEndPrice+10)) ? 
                    (stStockCtrl.ulGainPrice-ulLatestEndPrice) : 10;
    ulLossPriceDiff=(ulLatestEndPrice>(stStockCtrl.ulLossPrice+10)) ? 
                    (ulLatestEndPrice-stStockCtrl.ulLossPrice) : 10;
    printf("%06lu,%lu,%.2f,%.2f,%lu,%.2f,%lu\n",
           ulCode, stStockCtrl.ulBuyDate, FILE_PRICE2REAL(ulLatestEndPrice),
           FILE_PRICE2REAL(stStockCtrl.ulLossPrice), ulLossPriceDiff/10,
           FILE_PRICE2REAL(stStockCtrl.ulGainPrice), ulGainPriceDiff/10);

    return;
}

VOID CHOOSE_Distribute(IN ULONG ulCode, IN ULONG ulDate, IN CHAR * szDir, IN ULONG ulMethod)
{
    BOOL_T bIsDeal;
    CHOOSE_PRE_DEAL_S stDealInfo;
    ULONG ulEntryCnt;
    ULONG ulIndex;
    METHOD_FUNC_SET_S stMethodFunc;
    Choose_PF pfChoose = NULL;
    FILE_WHOLE_DATA_S *astWholeData = NULL;
    
    ulEntryCnt = FILE_GetFileData(ulCode, szDir, FILE_TYPE_CUSTOM, (VOID**)&astWholeData);
    if (0 == ulEntryCnt) return;

    ulIndex = GetIndexByDate(ulDate, INDEX_PREV, ulEntryCnt, astWholeData);
    if (INVAILD_ULONG == ulIndex) {
        free(astWholeData);
        DebugOutString("invalid date=%lu, code=%06lu\n", ulDate, ulCode);
        return;
    }

    // get choose function by method
    METHOD_GetFuncSet(ulMethod, &stMethodFunc);
    pfChoose = stMethodFunc.pfDailyChoose;
    stMethodFunc.pfSetParam(0, NULL);
    bIsDeal = pfChoose(ulIndex, &astWholeData[ulIndex], &stDealInfo);

    free(astWholeData);

    if (BOOL_TRUE == bIsDeal) {
        printf("%06lu,%u,%u,%u,%u,%.2f\n",
               ulCode, stDealInfo.bIsSell, stDealInfo.usDealHour, stDealInfo.usDealMin, 
               stDealInfo.bIsHigher, stDealInfo.fThresholdPrice);
    }
    else {
        DebugOutString("%06lu not in deal\n",ulCode);
    }

    return;
}

// choose wish list for all stocks where date is choose date
// track gain and loss price for single stock where date is buy date
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

    if (1 == ulCodeCnt) {
        CHOOSE_Track(pulCodeList[0], ulDate, argv[2], ulMethod);
    }
    else {
        for (i=0;i<ulCodeCnt;i++) {
            CHOOSE_Distribute(pulCodeList[i], ulDate, argv[2], ulMethod);
        }
    }
    
    return 0;
}

