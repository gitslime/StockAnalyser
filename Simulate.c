#include "common/comm.h"
#include "common/file.h"
#include "method/method.h"

typedef struct tagDealInfo
{
    FLOAT fTotalProfit;
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
    
    if (0 == ulTotalDeal) {
        printf("no deal info\n");
        return;
    }

    printf("\r\nTotal Profit=%f%%, Total Deal=%u, Average Profit=%f%%\n", 
           pstDealInfo->fTotalProfit*100, ulTotalDeal, pstDealInfo->fTotalProfit/ulTotalDeal*100);
    return;
}

FLOAT SIM_GetProfit(IN FILE_WHOLE_DATA_S *pstCurrData, IN STOCK_CTRL_S *pstStockCtrl)
{
    FLOAT fMulti, fAdder, fProfit;

    GetFactor(pstStockCtrl->ulBuyDate, pstCurrData, &fMulti, &fAdder);

    // take off deal cost 0.3%
    fProfit = ((pstStockCtrl->ulSellPrice * fMulti) + fAdder)/pstStockCtrl->ulBuyPrice - 1.003F;

    DebugOutString("%06u,%u,%u,%u,%u,%.3f\n",
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

    if (INVAILD_ULONG != pstStockCtrl->ulSellPrice) {
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

    ulMethod = GetMethod(szMethod);

    switch (ulMethod) {
        case METHOD_RISE:
            g_pfGetGainPrice = RISE_GetGainPrice;
            g_pfGetLossPrice = RISE_GetLossPrice;
            g_pfGetBuyPrice  = RISE_GetBuyPrice;
            g_pfGetSellPrice = RISE_GetSellPrice;
            g_pfDailyChoose  = RISE_Choose;
            
            break;
        default:
            printf("method not support\n");
            exit(5);
            break;
    }

    return;
}

int main(int argc,char *argv[])
{
    ULONG i, ulCodeCnt;
    ULONG ulIndex;
    BOOL_T bIsSell, bIsBuy, bIsWish;
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
                    stDealInfo.fTotalProfit += SIM_GetProfit(pstCurrData, pstStockCtrl);
                    stDealInfo.ulTotalDeal++;
                }
            }

            if (BOOL_FALSE != pstStockCtrl->bIsWish) {
                // handle Wish
                bIsBuy = SIM_HandleWish(pstCurrData, pstStockCtrl);
                pstStockCtrl->bIsWish = BOOL_FALSE;
            }

            // get wishlist
            bIsWish = g_pfDailyChoose(apstCurrData[i]-apstWholeData[i], apstCurrData[i], &stWish);
            if (BOOL_FALSE != bIsWish) {
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

