#include "common/comm.h"
#include "common/file.h"
#include "method/method.h"

typedef struct tagDealInfo
{
    FLOAT fTotalProfit;
    FLOAT fMaxProfit;
    FLOAT fMinProfit;
    ULONG ulTotalDeal;
    ULONG ulGainP1;     //  0%  <  profit <=    1%
    ULONG ulGainP3;     //  1%  <  profit <=    3%
    ULONG ulGainP5;     //  3%  <  profit <=    5%
    ULONG ulGainP10;    //  5%  <  profit <=   10%
    ULONG ulGainMore;   // 10%  <  profit
    ULONG ulLossP1;     // -1%  <  profit <=    0%
    ULONG ulLossP3;     // -3%  <  profit <=   -1%
    ULONG ulLossP5;     // -5%  <  profit <=   -3%
    ULONG ulLossP10;    //-10%  <  profit <=   -5%
    ULONG ulLossMore;   //         profit <=  -10%
}SIM_DEAL_INFO_S;

GetGainPrice_PF g_pfGetGainPrice = NULL;
GetLossPrice_PF g_pfGetLossPrice = NULL;
GetBuyPrice_PF  g_pfGetBuyPrice  = NULL;
GetSellPrice_PF g_pfGetSellPrice = NULL;
Choose_PF       g_pfDailyChoose  = NULL;

VOID SIM_PrintDealInfo(IN SIM_DEAL_INFO_S *pstDealInfo)
{
    ULONG ulTotalDeal = pstDealInfo->ulTotalDeal;
    FLOAT fTotalDeal  = ((FLOAT)ulTotalDeal/100);
    
    if (0 == ulTotalDeal) {
        printf("no deal info\n");
        return;
    }

    printf("\r\nTotalProfit=%.2f%%(%.2f%%~%.2f%%), TotalDeal=%u, AverageProfit=%.4f%%\n", 
           pstDealInfo->fTotalProfit*100, pstDealInfo->fMinProfit*100, pstDealInfo->fMaxProfit*100,
           ulTotalDeal, pstDealInfo->fTotalProfit/ulTotalDeal*100);
    printf("~-10%%]\t\t(-10%%,-5%%]\t(-5%%,-3%%]\t(-3%%,-1%%]\t(-1%%,0%%]\n");
    printf("%.2f%%\t\t%.2f%%\t\t%.2f%%\t\t%.2f%%\t\t%.2f%%\n",
           pstDealInfo->ulLossMore/fTotalDeal,  pstDealInfo->ulLossP10/fTotalDeal, 
           pstDealInfo->ulLossP5/fTotalDeal,    pstDealInfo->ulLossP3/fTotalDeal, 
           pstDealInfo->ulLossP1/fTotalDeal);

    
    printf("(0%%,1%%]\t\t(1%%,3%%]\t\t(3%%,5%%]\t\t(5%%,10%%]\t(10%%~\n");
    printf("%.2f%%\t\t%.2f%%\t\t%.2f%%\t\t%.2f%%\t\t%.2f%%\n",
           pstDealInfo->ulGainP1/fTotalDeal, 
           pstDealInfo->ulGainP3/fTotalDeal,    pstDealInfo->ulGainP5/fTotalDeal, 
           pstDealInfo->ulGainP10/fTotalDeal,   pstDealInfo->ulGainMore/fTotalDeal);

    return;
}

VOID SIM_SetDealInfo(IN FLOAT fProfit, OUT SIM_DEAL_INFO_S *pstDealInfo)
{
   if (fProfit <= -0.10F) {
        pstDealInfo->ulLossMore++;
    }
    else if ((-0.10F < fProfit) && (fProfit <= -0.05F)) {
        pstDealInfo->ulLossP10++;
    }
    else if ((-0.05F < fProfit) && (fProfit <= -0.03F)) {
        pstDealInfo->ulLossP5++;
    }
    else if ((-0.03F < fProfit) && (fProfit <= -0.01F)) {
        pstDealInfo->ulLossP3++;
    }
    else if ((-0.01F < fProfit) && (fProfit <= 0.00F)) {
        pstDealInfo->ulLossP1++;
    }
    else if ((0.00F < fProfit) && (fProfit <= 0.01F)) {
        pstDealInfo->ulGainP1++;
    }
    else if ((0.01F < fProfit) && (fProfit <= 0.03F)) {
        pstDealInfo->ulGainP3++;
    }
    else if ((0.03F < fProfit) && (fProfit <= 0.05F)) {
        pstDealInfo->ulGainP5++;
    }
    else if ((0.05F < fProfit) && (fProfit <= 0.10F)) {
        pstDealInfo->ulGainP10++;
    }
    else {
        pstDealInfo->ulGainMore++;
    }
    pstDealInfo->ulTotalDeal++;
    pstDealInfo->fTotalProfit+=fProfit;
    pstDealInfo->fMaxProfit=MAX(pstDealInfo->fTotalProfit, pstDealInfo->fMaxProfit);
    pstDealInfo->fMinProfit=MIN(pstDealInfo->fTotalProfit, pstDealInfo->fMinProfit);
    
    return;
}

FLOAT SIM_GetProfit(IN FILE_WHOLE_DATA_S *pstCurrData, IN STOCK_CTRL_S *pstStockCtrl)
{
    FLOAT fMulti, fAdder, fProfit;

    GetFactor(pstStockCtrl->ulBuyDate, pstCurrData, &fMulti, &fAdder);

    // take off deal cost 0.3%
    fProfit = ((pstStockCtrl->ulSellPrice * fMulti) + fAdder)/pstStockCtrl->ulBuyPrice - 1.003F;

    printf("%06u,%u,%u,%u,%u,%.3f\n",
        pstStockCtrl->ulCode, pstStockCtrl->ulBuyDate, pstStockCtrl->ulBuyPrice, 
        pstStockCtrl->ulSellDate, pstStockCtrl->ulSellPrice, fProfit);

    return fProfit;
}

// get sell price if reaching win and lost price in the same day
ULONG SIM_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, IN STOCK_CTRL_S *pstStockCtrl)
{
    ULONG i;
    ULONG ulSellPrice, ulGainPrice, ulLossPrice;
    PRICE_S *pstMin30Price = pstCurrData->astMin30Price;

    ulGainPrice  = pstStockCtrl->ulGainPrice;
    ulLossPrice = pstStockCtrl->ulLossPrice;
    ulSellPrice = INVAILD_ULONG;

    for (i=0;i<8;i++, pstMin30Price++) {
        if (FILE_VAILD_PRICE != pstMin30Price->ulFlag) break;

        if (pstMin30Price->ulHigh >= ulGainPrice) {
            if (pstMin30Price->ulLow <= ulLossPrice) {
                // reach win price and lost price in a same periord
                break;
            }
            else {
                // reach win price first
                ulSellPrice = ulGainPrice;
            }
        }
        else {
            if (pstMin30Price->ulLow <= ulLossPrice) {
                // reach lost price first
                ulSellPrice = ulLossPrice;
            }
            // none of win and lost price reached, check next periord
        }
   }
 
    // if cann't get sell price in 30 price, get a random price
    if (INVAILD_ULONG == ulSellPrice) {     
        // 50% for each win price and lost price
        ulSellPrice = (RandomUlong(0,2)==1) ? ulGainPrice : ulLossPrice;
    }

    return ulSellPrice;
}

BOOL_T SIM_HandleHold(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    BOOL_T bIsSell = BOOL_FALSE;
    BOOL_T bIsHigher, bIsLower;
    ULONG  ulSellPrice;

    bIsHigher = (pstCurrData->stDailyPrice.ulHigh >= pstStockCtrl->ulGainPrice) ? BOOL_TRUE : BOOL_FALSE;
    bIsLower  = (pstCurrData->stDailyPrice.ulLow  <= pstStockCtrl->ulLossPrice) ? BOOL_TRUE : BOOL_FALSE;

    if (BOOL_FALSE != bIsHigher) {
        if (BOOL_FALSE == bIsLower) {   // only reach win price
            if (pstCurrData->stDailyPrice.ulBegin > pstStockCtrl->ulGainPrice) {
                ulSellPrice = pstCurrData->stDailyPrice.ulBegin;
            }
            else {
                ulSellPrice = pstStockCtrl->ulGainPrice;
            }
        }
        else {                          // reach win price and lost price in the same day
            ulSellPrice = SIM_GetSellPrice(pstCurrData, pstStockCtrl);
        }
    }
    else {
        if (BOOL_FALSE == bIsLower) {   // reach none of win price and lost price
            ulSellPrice = g_pfGetSellPrice(pstCurrData, pstStockCtrl);
        }
        else {                          // only reach lost price
            if (pstCurrData->stDailyPrice.ulBegin < pstStockCtrl->ulLossPrice) {
                ulSellPrice = pstCurrData->stDailyPrice.ulBegin;
            }
            else {
                ulSellPrice = pstStockCtrl->ulLossPrice;
            }
        }
    }

    if (INVAILD_ULONG != ulSellPrice) {
        pstStockCtrl->ulSellDate  = pstCurrData->ulDate;
        pstStockCtrl->ulSellPrice = ulSellPrice;
        pstStockCtrl->bIsHold     = BOOL_FALSE;
        bIsSell = BOOL_TRUE;
    }

    return bIsSell;
}

BOOL_T SIM_HandleWish(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    ULONG ulBuyPrice;

    ulBuyPrice = g_pfGetBuyPrice(pstCurrData, pstStockCtrl);

    if (INVAILD_ULONG == ulBuyPrice) return BOOL_FALSE;

    pstStockCtrl->ulBuyDate   = pstCurrData->ulDate;
    pstStockCtrl->ulBuyPrice  = ulBuyPrice;
    pstStockCtrl->ulGainPrice = g_pfGetGainPrice(pstCurrData, pstStockCtrl);
    pstStockCtrl->ulLossPrice = g_pfGetLossPrice(pstCurrData, pstStockCtrl);
    pstStockCtrl->bIsWish     = BOOL_FALSE;
    pstStockCtrl->bIsHold     = BOOL_TRUE;

    return BOOL_TRUE;
}

VOID SIM_GetMethod(IN CHAR *szMethod)
{
    ULONG ulMethod;
    METHOD_FUNC_SET_S stMethodFunc;

    ulMethod = GetMethod(szMethod);

    // get functions by method
    METHOD_GetFuncSet(ulMethod, &stMethodFunc);
    g_pfGetGainPrice = stMethodFunc.pfGetGainPrice;
    g_pfGetLossPrice = stMethodFunc.pfGetLossPrice;
    g_pfGetBuyPrice  = stMethodFunc.pfGetBuyPrice; 
    g_pfGetSellPrice = stMethodFunc.pfGetSellPrice;
    g_pfDailyChoose  = stMethodFunc.pfDailyChoose;

    return;
}

#define SIM_TOTAL_SHARE_CNT       (4)

int main(int argc,char *argv[])
{
    ULONG i, ulCodeCnt;
    ULONG ulIndex;
    BOOL_T bIsSell, bIsBuy;
    FLOAT fProfit;
    ULONG ulShareCnt=0;
    SIM_DEAL_INFO_S stDealInfo;
    ULONG ulBeginDate, ulEndDate, ulCurrDate;
    ULONG *pulCodeList = NULL;
    CHOOSE_PRE_DEAL_S stWish;
    STOCK_CTRL_S *pstStockCtrl = NULL;
    STOCK_CTRL_S *astStockCtrl = NULL;
    ULONG aulEntryCnt[STOCK_TOTAL_CNT];
    FILE_WHOLE_DATA_S *pstCurrData;
    FILE_WHOLE_DATA_S *apstCurrData[STOCK_TOTAL_CNT];
    FILE_WHOLE_DATA_S *apstWholeData[STOCK_TOTAL_CNT];
    
    //check parameter
    if ((argc < 6) || (argc > 7)) {
        printf("USAGE: %s method path begin-date end-date { code | all } [debug]", argv[0]);
        exit(1);
    }
    SIM_GetMethod(argv[1]);

    RandomInit();

    if (0 == _stricmp(argv[argc-1], "debug"))
        g_bIsDebugMode = BOOL_TRUE;
    
    ulCodeCnt = GetCodeList(argv[5], &pulCodeList);
    ulBeginDate = (ULONG)atol(argv[3]);
    ulEndDate = (ULONG)atol(argv[4]);
    assert(ulBeginDate <= ulEndDate);
    assert(ulCodeCnt < STOCK_TOTAL_CNT);
    memset(&stDealInfo, 0, sizeof(stDealInfo));

    astStockCtrl = (STOCK_CTRL_S *)malloc(sizeof(STOCK_CTRL_S) * ulCodeCnt);
    assert(NULL != astStockCtrl);
    memset(astStockCtrl, 0, sizeof(STOCK_CTRL_S) * ulCodeCnt);

    // get all data
    memset(aulEntryCnt, 0, sizeof(aulEntryCnt));
    for (i=0;i<ulCodeCnt;i++) {
        aulEntryCnt[i] = FILE_GetFileData(pulCodeList[i], argv[2], FILE_TYPE_CUSTOM, &apstWholeData[i]);
        if (0 == aulEntryCnt[i]) {
            free(astStockCtrl);
            return;
        }
        astStockCtrl[i].ulCode = pulCodeList[i];
    }

    // get start point
    for (i=0;i<ulCodeCnt;i++) {
        ulIndex = GetIndexByDate(ulBeginDate,INDEX_NEXT,aulEntryCnt[i],apstWholeData[i]);
        if (INVAILD_ULONG == ulIndex) {
            apstCurrData[i] = NULL;
        }
        else {
            apstCurrData[i] = &apstWholeData[i][ulIndex];
        }
    }

    //step by date
    for (ulCurrDate=ulBeginDate; ulCurrDate<=ulEndDate; ulCurrDate++) {
        if (BOOL_FALSE == IsVaildDate(ulCurrDate)) continue;

        // in each day, check every stock
        for (i=0;i<ulCodeCnt;i++) {
            if (NULL == apstCurrData[i]) continue;
            if (ulCurrDate != apstCurrData[i]->ulDate) continue;

            pstCurrData  = apstCurrData[i];
            pstStockCtrl = &astStockCtrl[i];
            if (BOOL_FALSE != pstStockCtrl->bIsHold) {
                // handle hold
                bIsSell = SIM_HandleHold(pstCurrData, pstStockCtrl);
                if (BOOL_FALSE != bIsSell) {
                    // stock sold
                    fProfit = SIM_GetProfit(pstCurrData, pstStockCtrl);
                    SIM_SetDealInfo(fProfit, &stDealInfo);
                    ulShareCnt--;
                }
            }

            if (BOOL_FALSE != pstStockCtrl->bIsWish) {
                // handle Wish
                pstStockCtrl->bIsWish = BOOL_FALSE;
                if (ulShareCnt < SIM_TOTAL_SHARE_CNT) {
                    bIsBuy = SIM_HandleWish(pstCurrData, pstStockCtrl);
                    if (BOOL_FALSE != bIsBuy) ulShareCnt++;
                }
            }

            // get wishlist
            if ((BOOL_FALSE == pstStockCtrl->bIsHold) &&
                (BOOL_FALSE != g_pfDailyChoose(apstCurrData[i]-apstWholeData[i], apstCurrData[i], &stWish))) {
                pstStockCtrl->bIsWish     = BOOL_TRUE;
                pstStockCtrl->ulWishPrice = FILE_REAL2PRICE(stWish.fThresholdPrice);
            }
            
            apstCurrData[i]++;
        }
    }

    for (i=0;i<ulCodeCnt;i++) {
        if (0 != aulEntryCnt[i]) free(apstWholeData[i]);
    }
    free(astStockCtrl);

    SIM_PrintDealInfo(&stDealInfo);
    return 0;
}

