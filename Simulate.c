#include "common/comm.h"
#include "common/file.h"
#include "method/method.h"

#define SIM_HAND_LIMIT      (10000000)  //10000 RMB per hand

typedef struct tagCapLine
{
    ULONG ulDate;
    ULONG ulCapital;
}SIM_CAP_LINE_S;

typedef struct tagDealInfo
{
    SIM_CAP_LINE_S *astCapLine;
    FLOAT fTotalProfit;
    FLOAT fMaxProfit;
    FLOAT fMinProfit;
    ULONG ulFreeCap;
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

typedef struct tagStockData
{
    ULONG ulEntryCnt;
    STOCK_CTRL_S       stStockCtrl;
    FILE_WHOLE_DATA_S *pstCurrData;
    FILE_WHOLE_DATA_S *astWholeData;
}SIM_STOCK_DATA_S;

GetGainPrice_PF g_pfGetGainPrice = NULL;
GetLossPrice_PF g_pfGetLossPrice = NULL;
GetBuyPrice_PF  g_pfGetBuyPrice  = NULL;
GetSellPrice_PF g_pfGetSellPrice = NULL;
Choose_PF       g_pfDailyChoose  = NULL;
SortWishList_PF g_pfSortWishList = NULL;

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

VOID SIM_PrintCapInfo(IN ULONG ulDays, IN SIM_DEAL_INFO_S *pstDealInfo)
{
    ULONG i;
    SIM_CAP_LINE_S *pstCurr=pstDealInfo->astCapLine;

    for (i=0;i<ulDays;i++,pstCurr++) {
        printf("%u,%.2f\n", pstCurr->ulDate, FILE_PRICE2REAL(pstCurr->ulCapital));
    }
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
    FLOAT fHighRate, fLowRate;

    GetFactor(pstStockCtrl->ulBuyDate, pstCurrData, &fMulti, &fAdder);

    // take off deal cost 0.3%
    fProfit = ((pstStockCtrl->ulSellPrice * fMulti) + fAdder)/pstStockCtrl->ulBuyPrice - 1.003F;
    fHighRate = (((FLOAT)pstStockCtrl->ulHoldHigh)/pstStockCtrl->ulBuyPrice) - 1.00F;
    fLowRate  = (((FLOAT)pstStockCtrl->ulHoldLow )/pstStockCtrl->ulBuyPrice) - 1.00F;

    printf("%06u,%u,%u,%u,%.3f,%u,%.3f,%u,%u,%.3f\n",
        pstStockCtrl->ulCode, pstStockCtrl->ulBuyDate, pstStockCtrl->ulBuyPrice, 
        pstStockCtrl->ulHoldHigh, fHighRate, pstStockCtrl->ulHoldLow, fLowRate,
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

BOOL_T SIM_HandleHold(IN FILE_WHOLE_DATA_S *pstCurrData, 
                      INOUT STOCK_CTRL_S *pstStockCtrl, 
                      INOUT SIM_DEAL_INFO_S *pstDealInfo)
{
    BOOL_T bIsSell = BOOL_FALSE;
    BOOL_T bIsHigher, bIsLower;
    ULONG  ulSellPrice;
    FLOAT  fProfit;

    bIsHigher = (pstCurrData->stDailyPrice.ulHigh >= pstStockCtrl->ulGainPrice) ? BOOL_TRUE : BOOL_FALSE;
    bIsLower  = (pstCurrData->stDailyPrice.ulLow  <= pstStockCtrl->ulLossPrice) ? BOOL_TRUE : BOOL_FALSE;
    pstStockCtrl->ulHoldHigh = MAX(pstStockCtrl->ulHoldHigh, pstCurrData->stDailyPrice.ulHigh);
    pstStockCtrl->ulHoldLow  = MIN(pstStockCtrl->ulHoldLow,  pstCurrData->stDailyPrice.ulLow);
    pstStockCtrl->ulCurrPrice = pstCurrData->stDailyPrice.ulEnd;

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
                // drop limit can not sell
                if (pstCurrData->stDailyPrice.ulLow==pstCurrData->stDailyPrice.ulHigh){
                    ulSellPrice = INVAILD_ULONG;
                }
                else {
                    ulSellPrice = pstCurrData->stDailyPrice.ulBegin;
                }
            }
            else {
                ulSellPrice = pstStockCtrl->ulLossPrice;
            }
        }
    }

    if (INVAILD_ULONG != ulSellPrice) {
        // sell stock
        pstStockCtrl->ulSellDate  = pstCurrData->ulDate;
        pstStockCtrl->ulSellPrice = ulSellPrice;
        pstStockCtrl->bIsHold     = BOOL_FALSE;
        bIsSell = BOOL_TRUE;

        // record
        fProfit = SIM_GetProfit(pstCurrData, pstStockCtrl);
        pstDealInfo->ulFreeCap += (ULONG)((1+fProfit)*pstStockCtrl->ulHoldCnt*pstStockCtrl->ulBuyPrice);
    }
    else {      // update gain price and loss price
        pstStockCtrl->ulGainPrice = g_pfGetGainPrice(pstCurrData, pstStockCtrl);
        pstStockCtrl->ulLossPrice = g_pfGetLossPrice(pstCurrData, pstStockCtrl);
    }

    return bIsSell;
}

BOOL_T SIM_HandleWish(IN FILE_WHOLE_DATA_S *pstCurrData, 
                      INOUT STOCK_CTRL_S *pstStockCtrl, 
                      INOUT SIM_DEAL_INFO_S *pstDealInfo)
{
    ULONG ulBuyPrice;
    ULONG ulBuyCnt;
    ULONG ulTotalBuyPrice;

    // clear wish
    pstStockCtrl->bIsWish     = BOOL_FALSE;

    // get buy price
    ulBuyPrice = g_pfGetBuyPrice(pstCurrData, pstStockCtrl);
    if (INVAILD_ULONG == ulBuyPrice) return BOOL_FALSE;

    // check remain capital
    ulBuyCnt = GetBuyCnt(SIM_HAND_LIMIT, ulBuyPrice);
    ulTotalBuyPrice=ulBuyCnt*ulBuyPrice;
    if (pstDealInfo->ulFreeCap < ulTotalBuyPrice) return BOOL_FALSE;
    pstDealInfo->ulFreeCap-=ulTotalBuyPrice;

    pstStockCtrl->ulBuyDate   = pstCurrData->ulDate;
    pstStockCtrl->ulBuyPrice  = ulBuyPrice;
    pstStockCtrl->ulHoldCnt   = ulBuyCnt;
    pstStockCtrl->ulCurrPrice = pstCurrData->stDailyPrice.ulEnd;
    pstStockCtrl->ulGainPrice = g_pfGetGainPrice(pstCurrData, pstStockCtrl);
    pstStockCtrl->ulLossPrice = g_pfGetLossPrice(pstCurrData, pstStockCtrl);
    pstStockCtrl->bIsHold     = BOOL_TRUE;
    pstStockCtrl->ulHoldHigh  = pstCurrData->stDailyPrice.ulHigh;
    pstStockCtrl->ulHoldLow   = pstCurrData->stDailyPrice.ulLow;

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
    g_pfSortWishList = stMethodFunc.pfSortWishList;

    return;
}

int main(int argc,char *argv[])
{
    ULONG i, j, ulCodeCnt;
    ULONG ulIndex;
    ULONG ulShareCnt=0;
    ULONG ulShareMax=0;
    FLOAT fCapital;
    ULONG ulHoldCapital=0;
    SIM_CAP_LINE_S *pstCapLine;
    SIM_DEAL_INFO_S stDealInfo;
    ULONG ulBeginDate, ulEndDate, ulCurrDate, ulDayCnt, ulActualDayCnt=0;
    ULONG ulNextDate=INVAILD_ULONG;
    ULONG ulTempDate=INVAILD_ULONG;
    ULONG ulWishCnt=0;
    ULONG aulWishCodeList[STOCK_TOTAL_CNT];
    ULONG ulHoldCnt=0;
    ULONG *pulCodeList = NULL;
    CHOOSE_PRE_DEAL_S stWish;
    STOCK_CTRL_S *pstStockCtrl = NULL;
    SIM_STOCK_DATA_S *pstStockData = NULL;
    SIM_STOCK_DATA_S *astStockData = NULL;
    FILE_WHOLE_DATA_S *pstCurrData = NULL;
    
    //check parameter
    if ((argc < 7) || (argc > 8)) {
        printf("USAGE: %s method path begin-date end-date capital { code | all } [debug]", argv[0]);
        exit(1);
    }
    SIM_GetMethod(argv[1]);

    RandomInit();

    if (0 == _stricmp(argv[argc-1], "debug"))
        g_bIsDebugMode = BOOL_TRUE;

    /* init parameter */
    ulCodeCnt = GetCodeList(argv[6], &pulCodeList);
    ulBeginDate = (ULONG)atol(argv[3]);
    ulEndDate = (ULONG)atol(argv[4]);
    fCapital = (FLOAT)atol(argv[5]);
    assert(ulBeginDate <= ulEndDate);
    assert(ulCodeCnt < STOCK_TOTAL_CNT);
    memset(&stDealInfo, 0, sizeof(stDealInfo));
    stDealInfo.ulFreeCap=FILE_REAL2PRICE(fCapital);
    ulDayCnt = GetDateInterval(ulBeginDate, ulEndDate);

    //astStockCtrl = (STOCK_CTRL_S *)malloc(sizeof(STOCK_CTRL_S) * ulCodeCnt);
    //assert(NULL != astStockCtrl);
    //memset(astStockCtrl, 0, sizeof(STOCK_CTRL_S) * ulCodeCnt);

    stDealInfo.astCapLine = (SIM_CAP_LINE_S *)malloc(sizeof(SIM_CAP_LINE_S) * ulDayCnt);
    assert(NULL != stDealInfo.astCapLine);
    memset(stDealInfo.astCapLine, 0, sizeof(SIM_CAP_LINE_S) * ulDayCnt);
    pstCapLine=stDealInfo.astCapLine;

    astStockData = (SIM_STOCK_DATA_S *)malloc(sizeof(SIM_STOCK_DATA_S) * ulCodeCnt);
    assert(NULL != astStockData);
    memset(astStockData, 0, sizeof(SIM_STOCK_DATA_S) * ulCodeCnt);

    // get all data
    for (i=0;i<ulCodeCnt;i++) {
        pstStockData = &(astStockData[i]);
        pstStockData->ulEntryCnt = FILE_GetFileData(pulCodeList[i], argv[2], FILE_TYPE_CUSTOM, &(pstStockData->astWholeData));
        assert (0 != pstStockData->ulEntryCnt);
        
        astStockData[i].stStockCtrl.ulCode = pulCodeList[i];
    }

    // get start point
    for (i=0;i<ulCodeCnt;i++) {
        pstStockData = &(astStockData[i]);
        ulIndex = GetIndexByDate(ulBeginDate,INDEX_NEXT,pstStockData->ulEntryCnt,pstStockData->astWholeData);
        if (INVAILD_ULONG == ulIndex) {
            pstStockData->pstCurrData = NULL;
        }
        else {
            pstStockData->pstCurrData = &(pstStockData->astWholeData[ulIndex]);
            ulNextDate=MIN(pstStockData->pstCurrData->ulDate, ulNextDate);
        }
    }

    //step by date
    for (ulCurrDate=ulBeginDate; ulCurrDate<=ulEndDate; ulCurrDate++) {
        if (ulCurrDate!=ulNextDate) continue;
        ulActualDayCnt++;

        // in each day, check every stock
        for (i=0;i<ulCodeCnt;i++) {     // check hold
            pstStockData = &(astStockData[i]);
            pstStockCtrl = &(pstStockData->stStockCtrl);
            if (BOOL_FALSE == pstStockCtrl->bIsHold) continue;
            if (NULL == pstStockData->pstCurrData) continue;
            if (ulCurrDate != pstStockData->pstCurrData->ulDate) continue;

            SIM_HandleHold(pstStockData->pstCurrData, pstStockCtrl, &stDealInfo);

            // update capital still hold
            if (BOOL_FALSE != pstStockCtrl->bIsHold)
                ulHoldCapital += pstStockCtrl->ulHoldCnt * pstStockCtrl->ulCurrPrice;
        }

        for (i=0;i<ulWishCnt;i++) {     // check wish
            // search wish code
            ULONG ulStockCode = aulWishCodeList[i];
            for (j=0;j<ulCodeCnt;j++) {
                if (ulStockCode == astStockData[j].stStockCtrl.ulCode) break;
            }
            assert(j!=ulCodeCnt);

            // handle wish list
            pstStockData = &(astStockData[j]);
            pstStockCtrl = &(pstStockData->stStockCtrl);
            assert(BOOL_TRUE == pstStockData->stStockCtrl.bIsWish);
            if (NULL == pstStockData->pstCurrData) continue;
            if (ulCurrDate != pstStockData->pstCurrData->ulDate) continue;

            SIM_HandleWish(pstStockData->pstCurrData, pstStockCtrl, &stDealInfo);

            // update capital still hold
            if (BOOL_FALSE != pstStockCtrl->bIsHold)
                ulHoldCapital += pstStockCtrl->ulHoldCnt * pstStockCtrl->ulCurrPrice;
        }

        // update capital
        pstCapLine->ulDate=ulCurrDate;
        pstCapLine->ulCapital=ulHoldCapital+stDealInfo.ulFreeCap;
        pstCapLine++;
        ulWishCnt=0;
        ulHoldCapital=0;

        for (i=0;i<ulCodeCnt;i++) {     // get wishlist
            pstStockData = &(astStockData[i]);
            pstStockCtrl = &(pstStockData->stStockCtrl);
            if (NULL == pstStockData->pstCurrData) continue;
            if (ulCurrDate != pstStockData->pstCurrData->ulDate) continue;
            if (BOOL_FALSE != pstStockCtrl->bIsHold) continue;
            
            if (BOOL_FALSE != g_pfDailyChoose(pstStockData->pstCurrData-pstStockData->astWholeData, 
                                              pstStockData->pstCurrData, &stWish))
            {
                pstStockCtrl->bIsWish     = BOOL_TRUE;
                pstStockCtrl->ulWishPrice = FILE_REAL2PRICE(stWish.fThresholdPrice);
                aulWishCodeList[ulWishCnt] = pstStockCtrl->ulCode;
                ulWishCnt++;
            }
        }

        g_pfSortWishList(ulWishCnt, aulWishCodeList);   // sort wishlist
        
        for (i=0;i<ulCodeCnt;i++) {     // shift data
            pstStockData = &(astStockData[i]);
            if (NULL == pstStockData->pstCurrData) continue;
            if (pstStockData->pstCurrData->ulDate==ulCurrDate) pstStockData->pstCurrData++;
            if (0==pstStockData->pstCurrData->ulDate) continue;
            ulTempDate=MIN(pstStockData->pstCurrData->ulDate, ulTempDate);
        }
        ulNextDate=ulTempDate;
        ulTempDate=INVAILD_ULONG;
    }

    for (i=0;i<ulCodeCnt;i++) {
        if (0 != astStockData[i].ulEntryCnt) free(astStockData[i].astWholeData);
    }
    free(astStockData);

    SIM_PrintCapInfo(ulActualDayCnt, &stDealInfo);
    SIM_PrintDealInfo(&stDealInfo);
    free(stDealInfo.astCapLine);
    
    return 0;
}

