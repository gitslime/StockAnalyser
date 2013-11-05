#include "common/comm.h"
#include "common/file.h"
#include "method/method.h"

#define SIM_HAND_LIMIT      (10000000)  //10000 RMB per hand
#define SIM_MAX_WISH_CNT    (1000)
#define SIM_FEE_RATE        (0.0008F)
#define SIM_TAX_RATE        (0.0010F)
#define SIM_SLIP_RATE       (0.0050F)

typedef struct tagCmdOption
{
    CHAR *szMethod;
    CHAR *szDataPath;
    ULONG ulBeginDate;
    ULONG ulEndDate;
    UINT64 ulTotalCap;
    CHAR *szStock;
    BOOL_T bIsDebug;
    BOOL_T bIsVerbose;
    CHAR *szParam;
}SIM_OPTION_S;

// for record
typedef struct tagCapLine
{
    ULONG ulDate;
    UINT64 ulCapital;
    ULONG ulStockCnt;
    ULONG ulWishCnt;
}SIM_CAP_LINE_S;

// for summary
typedef struct tagSummaryInfo
{
    CHAR *szParam;
    ULONG ulTotalDealCnt;
    ULONG ulWinDealCnt;
    FLOAT fTotalProfit;
    FLOAT fDealProfitSum;
    FLOAT fMaxDealProfit;
    FLOAT fMinDealProfit;
    UINT64 ulMaxCapital;
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
    FLOAT fChooseRise;
    FLOAT fChooseVolRate;
    STOCK_CTRL_S stStockCtrl;
}SIM_HOLD_LIST_S;

typedef struct tagAccountInfo
{
    //SIM_CAP_LINE_S *astCapLine;
    UINT64 ulFreeCap;
    SLL_NODE_S stHoldHead;
    ULONG ulWishCnt;
    SIM_WISH_LIST_S astWishList[SIM_MAX_WISH_CNT];
}SIM_ACCOUNT_S;

GetGainPrice_PF g_pfGetGainPrice = NULL;
GetLossPrice_PF g_pfGetLossPrice = NULL;
GetBuyPrice_PF  g_pfGetBuyPrice  = NULL;
GetSellPrice_PF g_pfGetSellPrice = NULL;
Choose_PF       g_pfDailyChoose  = NULL;
SetParam_PF     g_pfSetParam     = NULL;
BOOL_T          g_bIsVerbose     = BOOL_FALSE;

SIM_SUMMARY_S g_stSimSummary;

VOID SIM_PrintSummary(IN SIM_SUMMARY_S *pstSummary)
{
    printf("%s,%lu,%.4f,%.4f,%.5f,%.4f,%.4f,%.2f,%.4f\n", pstSummary->szParam,
           pstSummary->ulTotalDealCnt, pstSummary->fTotalProfit, 
           (FLOAT)pstSummary->ulWinDealCnt/pstSummary->ulTotalDealCnt,
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
        printf("%lu,%.2f,%lu,%lu\n", 
            pstCurr->ulDate, FILE_PRICE2REAL(pstCurr->ulCapital), pstCurr->ulStockCnt, pstCurr->ulWishCnt);
    }
    return;
}

VOID SIM_PrintOneDeal(IN SIM_HOLD_LIST_S *pstDealInfo)
{
    ULONG ulBuyPrice=pstDealInfo->stStockCtrl.ulBuyPrice;
    FLOAT fProfit=(((FLOAT)pstDealInfo->ulSellCapital)/pstDealInfo->ulBuyCapital) - 1.00F;
    FLOAT fHighRate = (((FLOAT)pstDealInfo->ulHoldHigh)/ulBuyPrice) - 1.00F;
    FLOAT fLowRate  = (((FLOAT)pstDealInfo->ulHoldLow )/ulBuyPrice) - 1.00F;

    printf("%06lu,%lu,%lu,%lu,%lu,%.3f,%lu,%.3f,%lu,%lu,%.5f,%.5f,%.3f\n",
        pstDealInfo->ulCode, pstDealInfo->stStockCtrl.ulBuyDate, ulBuyPrice, pstDealInfo->ulHoldCnt,
        pstDealInfo->ulHoldHigh, fHighRate, pstDealInfo->ulHoldLow, fLowRate,
        pstDealInfo->stStockCtrl.ulSellDate, pstDealInfo->stStockCtrl.ulSellPrice, fProfit,
        pstDealInfo->fChooseRise, pstDealInfo->fChooseVolRate);

    return;
}

VOID SIM_RecordOneDeal(IN SIM_HOLD_LIST_S *pstDealInfo)
{
    FLOAT fProfit=(((FLOAT)pstDealInfo->ulSellCapital)/pstDealInfo->ulBuyCapital) - 1.00F;

    g_stSimSummary.ulTotalDealCnt++;
    if (fProfit > 0) g_stSimSummary.ulWinDealCnt++;
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
    UINT64 ulHoldCap = 0;
    SIM_HOLD_LIST_S *pstHoldStock;
    
    for (pstHoldStock=(SIM_HOLD_LIST_S*)pstAccount->stHoldHead.pNext;
         NULL!=pstHoldStock;
         pstHoldStock=(SIM_HOLD_LIST_S*)pstHoldStock->stNode.pNext) {
        ulHoldCap += pstHoldStock->ulHoldCnt * (UINT64)pstHoldStock->ulCurrPrice;
        ulStockCnt++;
    }

    pstCurrCap->ulCapital = ulHoldCap + pstAccount->ulFreeCap;
    pstCurrCap->ulStockCnt = ulStockCnt;
    pstCurrCap->ulWishCnt = pstAccount->ulWishCnt;

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

        // update factor
        pstAccount->ulFreeCap += SIM_UpdateFactor(pstCurrData, pstHoldStock);

        pstStockCtrl = &(pstHoldStock->stStockCtrl);
        if (INVAILD_ULONG == ulSellPrice) {
            // still hold
            pstStockCtrl->ulGainPrice = g_pfGetGainPrice(pstCurrData, pstStockCtrl);
            pstStockCtrl->ulLossPrice = g_pfGetLossPrice(pstCurrData, pstStockCtrl);
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
            if (BOOL_TRUE == g_bIsVerbose) SIM_PrintOneDeal(pstHoldStock);
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
    FILE_WHOLE_DATA_S *pstPrevData;
    STOCK_CTRL_S *pstStockCtrl;
    SIM_HOLD_LIST_S *pstHoldStock;

    assert(ulWishCnt<=SIM_MAX_WISH_CNT);
    
    for (i=0,pstWish=pstAccount->astWishList;i<ulWishCnt;i++,pstWish++) {
        // get data
        pstCurrData = astStockData[pstWish->ulIndex].pstCurrData;

        if (ulCurrDate != pstCurrData->ulDate) continue;

        // get buy price
        ulBuyPrice = g_pfGetBuyPrice(pstCurrData, pstWish->ulWishPrice);
        if (INVAILD_ULONG == ulBuyPrice) continue;
        ulBuyPrice = MIN(pstCurrData->stDailyPrice.ulHigh, (ULONG)(ulBuyPrice*(1+SIM_SLIP_RATE)));

        // check remain capital
        ulBuyCnt = GetBuyCnt(SIM_HAND_LIMIT, ulBuyPrice);
        if (0 == ulBuyCnt) continue;
        ulBuyCapital = (ULONG)(ulBuyCnt*ulBuyPrice*(1+SIM_FEE_RATE+SIM_TAX_RATE));
        if (pstAccount->ulFreeCap < ulBuyCapital) continue;

        // record
        astStockData[pstWish->ulIndex].bIsHold = BOOL_TRUE;
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
        pstPrevData=pstCurrData-1;
        GetTotalRise(1,pstCurrData,RISE_TYPE_END,&(pstHoldStock->fChooseRise));
        pstHoldStock->fChooseVolRate=GetVolRatio(pstPrevData);
        pstStockCtrl = &(pstHoldStock->stStockCtrl);
        pstStockCtrl->ulBuyDate = pstCurrData->ulDate;
        pstStockCtrl->ulBuyPrice  = ulBuyPrice;
        pstStockCtrl->fContext    = pstWish->fWeight;
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
            pstWishStock->fWeight = stWish.fWeight;
            pstWishStock++;
            ulWishCnt++;
            if (ulWishCnt==SIM_MAX_WISH_CNT) break;
        }
    }

    return ulWishCnt;
}

VOID SIM_SortWishList(IN ULONG ulStartIndex, IN ULONG ulEndIndex, INOUT SIM_WISH_LIST_S *astWishList)
{
    ULONG i=ulStartIndex, j=ulEndIndex;
    FLOAT fKey = astWishList[ulStartIndex].fWeight;
    SIM_WISH_LIST_S stTemp;
    memcpy(&stTemp, &(astWishList[ulStartIndex]), sizeof(SIM_WISH_LIST_S));

    while (i<j) {
        while (astWishList[j].fWeight < fKey && i<j) j--;
        if (i<j) memcpy(&(astWishList[i++]), &(astWishList[j]), sizeof(SIM_WISH_LIST_S));
        while (astWishList[i].fWeight > fKey && i<j) i++;
        if (i<j) memcpy(&(astWishList[j--]), &(astWishList[i]), sizeof(SIM_WISH_LIST_S));
    }
    memcpy(&(astWishList[i]), &stTemp, sizeof(SIM_WISH_LIST_S));

    if (i > (ulStartIndex+1)) SIM_SortWishList(ulStartIndex, i-1, astWishList);
    if (ulEndIndex > (i+1))   SIM_SortWishList(i+1, ulEndIndex, astWishList);

    return;
}

ULONG SIM_ShiftDate(IN ULONG ulCurrDate, IN ULONG ulCodeCnt, INOUT SIM_STOCK_DATA_S *pstAllStockData)
{
    ULONG i;
    ULONG ulNextDate=INVAILD_ULONG;
    SIM_STOCK_DATA_S *pstStockData = pstAllStockData;

    for (i=0; i<ulCodeCnt; i++, pstStockData++) {     
        if (NULL == pstStockData->pstCurrData) continue;
        if (pstStockData->pstCurrData->ulDate==ulCurrDate) pstStockData->pstCurrData++;
        if (pstStockData->pstCurrData->ulDate < ulCurrDate) continue;  //goto the end of data
        if (0==pstStockData->pstCurrData->ulDate) continue;     // bug?
        ulNextDate=MIN(pstStockData->pstCurrData->ulDate, ulNextDate);
    }

    return ulNextDate;
}

ULONG SIM_GetOption(IN ULONG ulArgCnt, IN CHAR **ppcArgv, OUT SIM_OPTION_S *pstOpt)
{
    ULONG i, ulOptCnt;

    // init option for default
    pstOpt->szMethod    = "mma";
    pstOpt->szDataPath  = "F:/StockAnalyser/database";
    pstOpt->ulBeginDate = 20000101UL;
    pstOpt->ulEndDate   = 20131231UL;
    pstOpt->ulTotalCap  = 100000ULL;
    pstOpt->szStock     = "all";
    pstOpt->bIsDebug    = BOOL_FALSE;
    pstOpt->bIsVerbose  = BOOL_FALSE;
    pstOpt->szParam     = "(13, 50)";

    for (i=1,ulOptCnt=0;i<ulArgCnt;i+=2) {
        if(ppcArgv[i][0] != '-') {
            printf("error input: param should starts with '-'\n");
            exit(0);
        }
        
        switch(ppcArgv[i][1])
        {
            case 'm':
                pstOpt->szMethod = ppcArgv[i+1];
                break;
            case 'p':
                pstOpt->szParam = ppcArgv[i+1];
                break;
            case 'd':
                pstOpt->szDataPath = ppcArgv[i+1];
                break;
            case 'b':
                pstOpt->ulBeginDate = (ULONG)atol(ppcArgv[i+1]);
                break;
            case 'e':
                pstOpt->ulEndDate = (ULONG)atol(ppcArgv[i+1]);
                break;
            case 'c':
                pstOpt->ulTotalCap = (UINT64)atol(ppcArgv[i+1]);
                break;
            case 's':
                pstOpt->szStock = ppcArgv[i+1];
                break;
            case 'D':
                pstOpt->bIsDebug = BOOL_TRUE;
                break;
            case 'V':
                pstOpt->bIsVerbose = BOOL_TRUE;
                break;
            default:
                printf("error input: unknown param\n");
                exit(0);
        }
        ulOptCnt++;
    }
    assert(pstOpt->ulBeginDate<pstOpt->ulEndDate);

    return ulOptCnt;
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
    g_pfSetParam     = stMethodFunc.pfSetParam;

    return;
}

// format is (param1,param2,...)
VOID SIM_SetParam(IN CHAR *szParam)
{
    ULONG ulStrLen;
    ULONG ulParamCnt;
    FLOAT afParam[50];
    CHAR szSingleParam[100];
    CHAR *p=szParam;

    assert(*p == '(');
    p++;

    for (ulStrLen=0,ulParamCnt=0;*p!='\0';p++) {
        if ((*p != ',')&&(*p!=')')) {
            ulStrLen++;
            continue;
        }

        memcpy(szSingleParam, p-ulStrLen,ulStrLen);
        szSingleParam[ulStrLen]='\0';
        afParam[ulParamCnt]=(FLOAT)atof(szSingleParam);
        ulParamCnt++;
        ulStrLen=0;
    }

    DebugOutString("%lu,%f,%f\n", ulParamCnt, afParam[0], afParam[1]);

    g_pfSetParam(ulParamCnt, afParam);

    return;
}

int main(int argc,char *argv[])
{
    ULONG i, ulCodeCnt;
    SIM_OPTION_S stOption;
    ULONG ulIndex;
    SIM_CAP_LINE_S *pstCapLine;
    SIM_CAP_LINE_S *astCapLine;
    SIM_ACCOUNT_S stAccount;
    ULONG ulCurrDate, ulDayCnt, ulActualDayCnt=0;
    ULONG ulNextDate=INVAILD_ULONG;
    ULONG *pulCodeList = NULL;
    SIM_STOCK_DATA_S *pstStockData = NULL;
    SIM_STOCK_DATA_S *astStockData = NULL;
    
    RandomInit();

    /* init parameter */
    SIM_GetOption(argc, argv, &stOption);
    g_bIsDebugMode = stOption.bIsDebug;
    g_bIsVerbose   = stOption.bIsVerbose;
    g_stSimSummary.szParam = stOption.szParam;
    SIM_GetMethod(stOption.szMethod);
    SIM_SetParam(stOption.szParam);
    ulCodeCnt = GetCodeList(stOption.szStock, &pulCodeList);
    assert(ulCodeCnt < STOCK_TOTAL_CNT);

    // init capital line
    ulDayCnt = GetDateInterval(stOption.ulBeginDate, stOption.ulEndDate);
    astCapLine = (SIM_CAP_LINE_S *)malloc(sizeof(SIM_CAP_LINE_S) * ulDayCnt);
    assert(NULL != astCapLine);
    memset(astCapLine, 0, sizeof(SIM_CAP_LINE_S) * ulDayCnt);
    pstCapLine=astCapLine;

    // init account
    memset(&stAccount, 0, sizeof(stAccount));
    stAccount.ulFreeCap=FILE_REAL2PRICE(stOption.ulTotalCap);
    stAccount.stHoldHead.pNext=NULL;

    // init stock data
    astStockData = (SIM_STOCK_DATA_S *)malloc(sizeof(SIM_STOCK_DATA_S) * ulCodeCnt);
    assert(NULL != astStockData);
    memset(astStockData, 0, sizeof(SIM_STOCK_DATA_S) * ulCodeCnt);

    // get all data
    for (i=0,pstStockData=astStockData;i<ulCodeCnt;i++,pstStockData++) {
        pstStockData->ulEntryCnt = FILE_GetFileData(pulCodeList[i], stOption.szDataPath, 
                                                    FILE_TYPE_CUSTOM, (VOID**)&(pstStockData->astWholeData));
        assert (0 != pstStockData->ulEntryCnt);
        
        pstStockData->ulStockCode = pulCodeList[i];
        ulIndex = GetIndexByDate(stOption.ulBeginDate,INDEX_NEXT,pstStockData->ulEntryCnt,pstStockData->astWholeData);
        if (INVAILD_ULONG == ulIndex) {
            pstStockData->pstCurrData = NULL;
        }
        else {
            pstStockData->pstCurrData = &(pstStockData->astWholeData[ulIndex]);
            ulNextDate=MIN(pstStockData->pstCurrData->ulDate, ulNextDate);
        }
    }

    //step by date
    for (ulCurrDate=stOption.ulBeginDate; ulCurrDate<=stOption.ulEndDate; ulCurrDate++) {
        if (ulCurrDate!=ulNextDate) continue;
        ulActualDayCnt++;

        // check hold
        SIM_HandleHoldList(ulCurrDate, astStockData, &stAccount);

        // check wish
        SIM_HandleWishList(ulCurrDate, astStockData, &stAccount);

        // get wish list
        stAccount.ulWishCnt = SIM_GetWishList(ulCurrDate, ulCodeCnt, astStockData, stAccount.astWishList);

        // sort wishlist
        if (stAccount.ulWishCnt > 0)
            SIM_SortWishList(0, stAccount.ulWishCnt-1, stAccount.astWishList);

        // update capital
        pstCapLine->ulDate=ulCurrDate;
        SIM_UpdateCapital(&stAccount, pstCapLine);
        pstCapLine++;

        // shift data
        ulNextDate = SIM_ShiftDate(ulCurrDate, ulCodeCnt, astStockData);
    }
    // print capital line
    if (BOOL_TRUE == g_bIsVerbose)
        SIM_PrintCapInfo(ulActualDayCnt, astCapLine);

    // print summary
    pstCapLine--;
    g_stSimSummary.fTotalProfit=(FLOAT)pstCapLine->ulCapital/FILE_REAL2PRICE(stOption.ulTotalCap)-1.00F;
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

