#include "common/comm.h"
#include "common/file.h"
#include "method/method.h"

#define SIM_HAND_LIMIT      (10000000)  //10000 RMB per hand
#define SIM_MAX_WISH_CNT    (500)
#define SIM_FEE_RATE        (0.0008F)
#define SIM_TAX_RATE        (0.0010F)
#define SIM_SLIP_RATE       (0.0050F)

// for record
typedef struct tagCapLine
{
    ULONG ulDate;
    ULONG ulCapital;
    ULONG ulStockCnt;
}SIM_CAP_LINE_S;

// for summary
typedef struct tagSummaryInfo
{
    ULONG ulTotalDealCnt;
    FLOAT fTotalProfit;
    FLOAT fDealProfitSum;
    FLOAT fMaxDealProfit;
    FLOAT fMinDealProfit;
    ULONG ulMaxCapital;
    FLOAT fMaxCapWithdraw;
}SIM_SUMMARY_S;

typedef struct tagHoldList
{
    SLL_NODE_S stNode;
    ULONG ulCode;
    ULONG ulIndex;
    ULONG ulHoldCnt;
    ULONG ulCurrPrice;
    ULONG ulHoldLow;
    ULONG ulHoldHigh;
    ULONG ulBuyCapital;
    ULONG ulSellCapital;
    STOCK_CTRL_S stStockCtrl;
}SIM_HOLD_LIST_S;

typedef struct tagWishList
{
    ULONG ulCode;
    ULONG ulIndex;
    ULONG ulWishPrice;
}SIM_WISH_LIST_S;

typedef struct tagAccountInfo
{
    //SIM_CAP_LINE_S *astCapLine;
    ULONG ulFreeCap;
    SLL_NODE_S stHoldHead;
    ULONG ulWishCnt;
    SIM_WISH_LIST_S astWishList[SIM_MAX_WISH_CNT];
}SIM_ACCOUNT_S;

typedef struct tagStockData
{
    ULONG ulStockCode;
    ULONG ulEntryCnt;
    BOOL_T bIsHold;     // for get wish list to skip hold stocks
    FILE_WHOLE_DATA_S *pstCurrData;
    FILE_WHOLE_DATA_S *astWholeData;
}SIM_STOCK_DATA_S;

GetGainPrice_PF g_pfGetGainPrice = NULL;
GetLossPrice_PF g_pfGetLossPrice = NULL;
GetBuyPrice_PF  g_pfGetBuyPrice  = NULL;
GetSellPrice_PF g_pfGetSellPrice = NULL;
Choose_PF       g_pfDailyChoose  = NULL;
SortWishList_PF g_pfSortWishList = NULL;
SIM_SUMMARY_S g_stSimSummary;

VOID SIM_PrintSummary(IN SIM_SUMMARY_S *pstSummary)
{
    printf("%u,%.5f,%.5f,%.5f,%.5f,%.2f,%.4f\n",
           pstSummary->ulTotalDealCnt, pstSummary->fTotalProfit, 
           pstSummary->fDealProfitSum/pstSummary->ulTotalDealCnt,
           pstSummary->fMaxDealProfit, pstSummary->fMinDealProfit,
           FILE_PRICE2REAL(pstSummary->ulMaxCapital), pstSummary->fMaxCapWithdraw);

    return;
}

VOID SIM_PrintCapInfo(IN ULONG ulDays, IN SIM_CAP_LINE_S *pstCapLine)
{
    ULONG i;
    SIM_CAP_LINE_S *pstCurr=pstCapLine;

    for (i=0;i<ulDays;i++,pstCurr++) {
        printf("%u,%.2f,%u\n", pstCurr->ulDate, FILE_PRICE2REAL(pstCurr->ulCapital), pstCurr->ulStockCnt);
    }
    return;
}

VOID SIM_PrintOneDeal(IN SIM_HOLD_LIST_S *pstDealInfo)
{
    ULONG ulBuyPrice=pstDealInfo->stStockCtrl.ulBuyPrice;
    FLOAT fProfit=(((FLOAT)pstDealInfo->ulSellCapital)/pstDealInfo->ulBuyCapital) - 1.00F;
    FLOAT fHighRate = (((FLOAT)pstDealInfo->ulHoldHigh)/ulBuyPrice) - 1.00F;
    FLOAT fLowRate  = (((FLOAT)pstDealInfo->ulHoldLow )/ulBuyPrice) - 1.00F;

    printf("%06u,%u,%u,%u,%u,%.3f,%u,%.3f,%u,%u,%.5f\n",
        pstDealInfo->ulCode, pstDealInfo->stStockCtrl.ulBuyDate, ulBuyPrice, pstDealInfo->ulHoldCnt,
        pstDealInfo->ulHoldHigh, fHighRate, pstDealInfo->ulHoldLow, fLowRate,
        pstDealInfo->stStockCtrl.ulSellDate, pstDealInfo->stStockCtrl.ulSellPrice, fProfit);

    return;
}

VOID SIM_RecordOneDeal(IN SIM_HOLD_LIST_S *pstDealInfo)
{
    FLOAT fProfit=(((FLOAT)pstDealInfo->ulSellCapital)/pstDealInfo->ulBuyCapital) - 1.00F;

    g_stSimSummary.ulTotalDealCnt++;
    g_stSimSummary.fDealProfitSum+=fProfit;
    g_stSimSummary.fMaxDealProfit=MAX(g_stSimSummary.fMaxDealProfit, fProfit);
    g_stSimSummary.fMinDealProfit=MIN(g_stSimSummary.fMinDealProfit, fProfit);

    return;
}

// get sell price if reaching win and lost price in the same day
ULONG SIM_JudgeSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, IN STOCK_CTRL_S *pstStockCtrl)
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

ULONG SIM_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, IN SIM_HOLD_LIST_S *pstHoldStock)
{
    BOOL_T bIsHigher, bIsLower;
    ULONG  ulSellPrice;
    STOCK_CTRL_S *pstStockCtrl = &(pstHoldStock->stStockCtrl);

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
            ulSellPrice = SIM_JudgeSellPrice(pstCurrData, pstStockCtrl);
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

    return ulSellPrice;
}

ULONG SIM_UpdateFactor(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT SIM_HOLD_LIST_S *pstHoldStock)
{
    ULONG ulBonus;
    
    if (FILE_VAILD_FACTOR != pstCurrData->stFactor.ulFlag) return 0;

    if (pstCurrData->stFactor.fAdder < 0) return 0;     // not deal offer

    ulBonus = (ULONG)((pstHoldStock->ulHoldCnt) * (pstCurrData->stFactor.fAdder));
    pstHoldStock->ulSellCapital += ulBonus;
    pstHoldStock->ulHoldCnt = (ULONG)(pstHoldStock->ulHoldCnt*pstCurrData->stFactor.fMulti);

    return ulBonus;
}

VOID SIM_UpdateCapital(IN SIM_ACCOUNT_S *pstAccount, OUT SIM_CAP_LINE_S *pstCurrCap)
{
    ULONG ulStockCnt = 0;
    ULONG ulHoldCap = 0;
    SIM_HOLD_LIST_S *pstHoldStock;
    
    for (pstHoldStock=(SIM_HOLD_LIST_S*)pstAccount->stHoldHead.pNext;
         NULL!=pstHoldStock;
         pstHoldStock=(SIM_HOLD_LIST_S*)pstHoldStock->stNode.pNext) {
        ulHoldCap += pstHoldStock->ulHoldCnt * pstHoldStock->ulCurrPrice;
        ulStockCnt++;
    }

    pstCurrCap->ulCapital = ulHoldCap + pstAccount->ulFreeCap;
    pstCurrCap->ulStockCnt = ulStockCnt;

    g_stSimSummary.ulMaxCapital=MAX(g_stSimSummary.ulMaxCapital, pstCurrCap->ulCapital);
    g_stSimSummary.fMaxCapWithdraw=MIN(g_stSimSummary.fMaxCapWithdraw, 
        ((((FLOAT)pstCurrCap->ulCapital)/g_stSimSummary.ulMaxCapital) - 1.00F));
    
    return;
}

VOID SIM_HandleHoldList(IN ULONG ulCurrDate, IN SIM_STOCK_DATA_S *astStockData, INOUT SIM_ACCOUNT_S *pstAccount)
{
    ULONG ulSellPrice;
    SLL_NODE_S *pstPrev;
    SIM_HOLD_LIST_S *pstHoldStock;
    FILE_WHOLE_DATA_S *pstCurrData;
    STOCK_CTRL_S *pstStockCtrl;

    // node maybe deleted in the loop, so use prev for loop
    for (pstPrev=&(pstAccount->stHoldHead);
         (NULL!=pstPrev) && (NULL != pstPrev->pNext);
         pstPrev=(SLL_NODE_S *)(pstPrev->pNext)) {
        // get hold data
        pstHoldStock = (SIM_HOLD_LIST_S *)(pstPrev->pNext);
        pstCurrData = astStockData[pstHoldStock->ulIndex].pstCurrData;

        if (ulCurrDate != pstCurrData->ulDate) continue;
    
        ulSellPrice = SIM_GetSellPrice(pstCurrData, pstHoldStock);
    
        // update record
        pstHoldStock->ulHoldHigh = MAX(pstHoldStock->ulHoldHigh, pstCurrData->stDailyPrice.ulHigh);
        pstHoldStock->ulHoldLow  = MIN(pstHoldStock->ulHoldLow,  pstCurrData->stDailyPrice.ulLow);
        pstHoldStock->ulCurrPrice = pstCurrData->stDailyPrice.ulEnd;

        pstStockCtrl = &(pstHoldStock->stStockCtrl);
        if (INVAILD_ULONG == ulSellPrice) {
            // still hold
            pstStockCtrl->ulGainPrice = g_pfGetGainPrice(pstCurrData, pstStockCtrl);
            pstStockCtrl->ulLossPrice = g_pfGetLossPrice(pstCurrData, pstStockCtrl);

            pstAccount->ulFreeCap += SIM_UpdateFactor(pstCurrData, pstHoldStock);
        }
        else {
            // sell stock
            ULONG ulSellCap=(ULONG)(ulSellPrice*(pstHoldStock->ulHoldCnt)*(1-SIM_TAX_RATE-SIM_FEE_RATE));

            pstStockCtrl->ulSellDate  = pstCurrData->ulDate;
            pstStockCtrl->ulSellPrice = ulSellPrice;
            
            // record
            pstHoldStock->ulSellCapital+=ulSellCap;
            pstAccount->ulFreeCap += ulSellCap;
            astStockData[pstHoldStock->ulIndex].bIsHold = BOOL_FALSE;
            SIM_PrintOneDeal(pstHoldStock);
            SIM_RecordOneDeal(pstHoldStock);

            SLL_DeleteNode(&(pstAccount->stHoldHead), (SLL_NODE_S *)pstHoldStock);
            free(pstHoldStock);
        }
    }
    
    return;
}

VOID SIM_HandleWishList(IN ULONG ulCurrDate, IN SIM_STOCK_DATA_S *astStockData, INOUT SIM_ACCOUNT_S *pstAccount)
{
    ULONG i;
    ULONG ulBuyPrice, ulBuyCnt;
    ULONG ulBuyCapital;
    ULONG ulWishCnt=pstAccount->ulWishCnt;
    SIM_WISH_LIST_S *pstWish;
    FILE_WHOLE_DATA_S *pstCurrData;
    STOCK_CTRL_S *pstStockCtrl;
    SIM_HOLD_LIST_S *pstHoldStock;

    assert(ulWishCnt<SIM_MAX_WISH_CNT);
    
    for (i=0,pstWish=pstAccount->astWishList;i<ulWishCnt;i++,pstWish++) {
        // get data
        pstCurrData = astStockData[pstWish->ulIndex].pstCurrData;

        if (ulCurrDate != pstCurrData->ulDate) continue;

        // get buy price
        ulBuyPrice = g_pfGetBuyPrice(pstCurrData, pstWish->ulWishPrice);
        if (INVAILD_ULONG == ulBuyPrice) continue;
        ulBuyPrice = (ULONG)(ulBuyPrice*(1+SIM_SLIP_RATE));

        // check remain capital
        ulBuyCnt = GetBuyCnt(SIM_HAND_LIMIT, ulBuyPrice);
        if (0 == ulBuyCnt) continue;
        ulBuyCapital = (ULONG)(ulBuyCnt*ulBuyPrice*(1+SIM_FEE_RATE+SIM_TAX_RATE));
        if (pstAccount->ulFreeCap < ulBuyCapital) continue;

        // record
        pstAccount->ulFreeCap -= ulBuyCapital;
        pstHoldStock = (SIM_HOLD_LIST_S *)malloc(sizeof(SIM_HOLD_LIST_S));
        memset(pstHoldStock, 0, sizeof(SIM_HOLD_LIST_S));
        pstHoldStock->ulCode = pstWish->ulCode;
        pstHoldStock->ulIndex = pstWish->ulIndex;
        pstHoldStock->ulHoldCnt = ulBuyCnt;
        pstHoldStock->ulCurrPrice = pstCurrData->stDailyPrice.ulEnd;
        pstHoldStock->ulHoldLow = pstCurrData->stDailyPrice.ulLow;
        pstHoldStock->ulHoldHigh = pstCurrData->stDailyPrice.ulHigh;
        pstHoldStock->ulBuyCapital = ulBuyCapital;
        pstStockCtrl = &(pstHoldStock->stStockCtrl);
        pstStockCtrl->ulBuyDate = pstCurrData->ulDate;
        pstStockCtrl->ulBuyPrice  = ulBuyPrice;
        pstStockCtrl->ulGainPrice = g_pfGetGainPrice(pstCurrData, pstStockCtrl);
        pstStockCtrl->ulLossPrice = g_pfGetLossPrice(pstCurrData, pstStockCtrl);
        SLL_InsertInTail(&(pstAccount->stHoldHead), (SLL_NODE_S *)pstHoldStock);
    }
    
    return;
}

ULONG SIM_GetWishList(IN ULONG ulCurrDate, IN ULONG ulCodeCnt, 
                      IN SIM_STOCK_DATA_S *pstAllStockData, OUT SIM_WISH_LIST_S *astWishList)
{
    ULONG i;
    ULONG ulWishCnt=0;
    CHOOSE_PRE_DEAL_S stWish;
    SIM_STOCK_DATA_S *pstStockData = pstAllStockData;
    SIM_WISH_LIST_S *pstWishStock = astWishList;

    for (i=0;i<ulCodeCnt;i++, pstStockData++) {
        if (NULL == pstStockData->pstCurrData) continue;
        if (ulCurrDate != pstStockData->pstCurrData->ulDate) continue;
        if (BOOL_FALSE != pstStockData->bIsHold) continue;

        if (BOOL_FALSE != g_pfDailyChoose(pstStockData->pstCurrData-pstStockData->astWholeData, 
                                          pstStockData->pstCurrData, &stWish))
        {
            pstWishStock->ulCode = pstStockData->ulStockCode;
            pstWishStock->ulIndex = i;
            pstWishStock->ulWishPrice = FILE_REAL2PRICE(stWish.fThresholdPrice);
            pstWishStock++;
            ulWishCnt++;
            if (ulWishCnt==SIM_MAX_WISH_CNT) break;
        }
    }

    return ulWishCnt;
}

ULONG SIM_ShiftDate(IN ULONG ulCurrDate, IN ULONG ulCodeCnt, INOUT SIM_STOCK_DATA_S *pstAllStockData)
{
    ULONG i;
    ULONG ulNextDate=INVAILD_ULONG;
    SIM_STOCK_DATA_S *pstStockData = pstAllStockData;

    for (i=0; i<ulCodeCnt; i++, pstStockData++) {     
        if (NULL == pstStockData->pstCurrData) continue;
        if (pstStockData->pstCurrData->ulDate==ulCurrDate) pstStockData->pstCurrData++;
        if (0==pstStockData->pstCurrData->ulDate) continue;     // bug?
        ulNextDate=MIN(pstStockData->pstCurrData->ulDate, ulNextDate);
    }

    return ulNextDate;
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
    ULONG i, ulCodeCnt;
    ULONG ulIndex;
    SIM_CAP_LINE_S *pstCapLine;
    SIM_CAP_LINE_S *astCapLine;
    SIM_ACCOUNT_S stAccount;
    ULONG ulBeginDate, ulEndDate, ulCurrDate, ulDayCnt, ulActualDayCnt=0;
    ULONG ulNextDate=INVAILD_ULONG;
    ULONG *pulCodeList = NULL;
    SIM_STOCK_DATA_S *pstStockData = NULL;
    SIM_STOCK_DATA_S *astStockData = NULL;
    
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
    assert(ulBeginDate <= ulEndDate);
    assert(ulCodeCnt < STOCK_TOTAL_CNT);

    // init capital line
    ulDayCnt = GetDateInterval(ulBeginDate, ulEndDate);
    astCapLine = (SIM_CAP_LINE_S *)malloc(sizeof(SIM_CAP_LINE_S) * ulDayCnt);
    assert(NULL != astCapLine);
    memset(astCapLine, 0, sizeof(SIM_CAP_LINE_S) * ulDayCnt);
    pstCapLine=astCapLine;

    // init account
    memset(&stAccount, 0, sizeof(stAccount));
    stAccount.ulFreeCap=FILE_REAL2PRICE(atol(argv[5]));
    stAccount.stHoldHead.pNext=NULL;

    // init stock data
    astStockData = (SIM_STOCK_DATA_S *)malloc(sizeof(SIM_STOCK_DATA_S) * ulCodeCnt);
    assert(NULL != astStockData);
    memset(astStockData, 0, sizeof(SIM_STOCK_DATA_S) * ulCodeCnt);

    // get all data
    for (i=0,pstStockData=astStockData;i<ulCodeCnt;i++,pstStockData++) {
        pstStockData->ulEntryCnt = (USHORT)FILE_GetFileData(pulCodeList[i], argv[2], FILE_TYPE_CUSTOM, &(pstStockData->astWholeData));
        assert (0 != pstStockData->ulEntryCnt);
        
        pstStockData->ulStockCode = pulCodeList[i];
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

        // check hold
        SIM_HandleHoldList(ulCurrDate, astStockData, &stAccount);

        // check wish
        SIM_HandleWishList(ulCurrDate, astStockData, &stAccount);

        // update capital
        pstCapLine->ulDate=ulCurrDate;
        SIM_UpdateCapital(&stAccount, pstCapLine);
        pstCapLine++;

        // get wish list
        stAccount.ulWishCnt = SIM_GetWishList(ulCurrDate, ulCodeCnt, astStockData, stAccount.astWishList);

        //g_pfSortWishList(ulWishCnt, stAccount.astWishList);   // sort wishlist

        // shift data
        ulNextDate = SIM_ShiftDate(ulCurrDate, ulCodeCnt, astStockData);
    }
    // print capital line
    SIM_PrintCapInfo(ulActualDayCnt, astCapLine);

    // print summary
    pstCapLine--;
    g_stSimSummary.fTotalProfit=(FLOAT)pstCapLine->ulCapital/FILE_REAL2PRICE(atol(argv[5]))-1.00F;
    SIM_PrintSummary(&g_stSimSummary);

    // free memory
    SLL_FreeAll(&(stAccount.stHoldHead));
    for (i=0;i<ulCodeCnt;i++) {
        if (0 != astStockData[i].ulEntryCnt) free(astStockData[i].astWholeData);
    }
    free(astStockData);
    free(astCapLine);
    
    return 0;
}

