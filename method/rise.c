#include "../common/comm.h"
#include "../common/file.h"

#define RISE_WATCH_DAYS             (3)     // days watching to judge whether to buy
#define RISE_HOLD_THRESHOLD         (0.093F)

ULONG RISE_GetMinIndex(VOID)
{
    return RISE_WATCH_DAYS;
}

BOOL_T RISE_Statistics(IN FILE_WHOLE_DATA_S *pstSettleData)
{
    ULONG i;
    FLOAT afRise[RISE_WATCH_DAYS];
    FILE_WHOLE_DATA_S *pstWatch = pstSettleData-RISE_WATCH_DAYS+1;

    // get each rise rate
    for (i=0;i<RISE_WATCH_DAYS;i++,pstWatch++) {
        (VOID)GetTotalRise(1, pstWatch, RISE_TYPE_END, &afRise[i]);
    }

    if (afRise[0]<0) return BOOL_FALSE;
    for (i=1;i<RISE_WATCH_DAYS;i++,pstWatch++) {
        // make sure rise higher and higher
        if (afRise[i]<afRise[i-1]) return BOOL_FALSE;
    }
    printf("%d,", pstSettleData->ulDate);
    for (i=0;i<RISE_WATCH_DAYS;i++,pstWatch++) {
        printf("%.4f,", afRise[i]);
    }

    return BOOL_TRUE;
}

#define RISE_CHOOSE_DAYS        (2)
#define RISE_THRESHOLD_RATE     (1.10F)
#define RISE_THRESHOLD_DAILY    (1.02F)
#define RISE_BUY_HOUR           (14)

BOOL_T RISE_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    FLOAT  fPrevRise, fCurrRise;
    FLOAT  fThreshPrice;
    ULONG  ulMeanPrice=INVAILD_ULONG;
    FILE_WHOLE_DATA_S *pstBase = pstCurrData - RISE_CHOOSE_DAYS;
    FILE_WHOLE_DATA_S *pstTemp = pstBase-1;

    // make sure array is not out of bound
    if (ulIndex <= (RISE_CHOOSE_DAYS)) return BOOL_FALSE;

    #if 0
    if (ulIndex>(RISE_CHOOSE_DAYS+1)) {
        (VOID)GetTotalRise(1, pstBase, RISE_TYPE_END, &fPrevRise);
        if (fPrevRise > 0) return BOOL_FALSE;
    }
    #endif

#if 0
    ulMeanPrice = GetMean(RISE_MEAN_DAYS, pstBase, ulMeanPrice);
    if (pstBase->stDailyPrice.ulEnd>ulMeanPrice) return BOOL_FALSE;

    {
        ULONG i;
        ULONG ulNextMean;
        for (i=1;i<=RISE_CHOOSE_DAYS;i++) {
            ulNextMean=GetMean(RISE_MEAN_DAYS, (pstBase+i), ulMeanPrice);
            if (ulNextMean>=ulMeanPrice) return BOOL_FALSE;
            ulMeanPrice=ulNextMean;
        }
    }
    #endif
    //GetTotalRise(1, pstCurrData-1, RISE_TYPE_END, &fPrevRise);
    //if (fPrevRise<0) return BOOL_FALSE;
    //GetTotalRise(1, pstCurrData, RISE_TYPE_END, &fCurrRise);
    //if (fCurrRise < fPrevRise) return BOOL_FALSE;

    // continuously rise by choose days
    {
        BOOL_T bIsContinuous;
        bIsContinuous = GetTotalRise(RISE_CHOOSE_DAYS, pstCurrData, RISE_TYPE_END, &fPrevRise);
        if (BOOL_FALSE == bIsContinuous) return BOOL_FALSE;
    }

    //if (fPrevRise > 0.15) return BOOL_FALSE;

    pstDealInfo->bIsSell    = BOOL_FALSE;
    pstDealInfo->usDealHour = RISE_BUY_HOUR;
    pstDealInfo->usDealMin  = (USHORT)RandomUlong(50,60);
    pstDealInfo->bIsHigher  = BOOL_TRUE;

    // make sure rising continuously
    //fThreshPrice = (1+fCurrRise)*pstCurrData->stDailyPrice.ulEnd;
    fThreshPrice = MAX((RISE_THRESHOLD_RATE  * pstBase->stDailyPrice.ulEnd),
                       (RISE_THRESHOLD_DAILY * pstCurrData->stDailyPrice.ulEnd));
    
    pstDealInfo->fThresholdPrice = FILE_PRICE2REAL(fThreshPrice);

    return BOOL_TRUE;
}

#define RISE_HOLD_DAYS           (1)            // days hold a stock
#define RISE_GAIN_THRESHOLD      (1.033F) 
#define RISE_LOSS_THRESHOLD      (0.923F)

ULONG RISE_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    ULONG ulHoldDays=0;
    FILE_WHOLE_DATA_S *pstData=pstCurrData;

    assert(pstCurrData->ulDate>=pstStockCtrl->ulBuyDate);

    // point to buy date
    while (1) {
        ulHoldDays++;
        if (pstData->ulDate==pstStockCtrl->ulBuyDate) break;
        pstData--;
    }

    return (ULONG)(pstData->stDailyPrice.ulEnd * RISE_GAIN_THRESHOLD);
}

ULONG RISE_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    ULONG ulHoldDays=0;
    FILE_WHOLE_DATA_S *pstData=pstCurrData;

    assert(pstCurrData->ulDate>=pstStockCtrl->ulBuyDate);

    // point to buy date
    while (1) {
        ulHoldDays++;
        if (pstData->ulDate==pstStockCtrl->ulBuyDate) break;
        pstData--;
    }

    return (ULONG)(pstData->stDailyPrice.ulEnd * RISE_LOSS_THRESHOLD);
}

ULONG RISE_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    FLOAT fRise;

    // get today's rise
    (VOID)GetTotalRise(1,pstCurrData,RISE_TYPE_END,&fRise);

    if (fRise > STOCK_RISE_THREASHOLD) return INVAILD_ULONG;   // limit up cann't buy

    // this check is for ST
    if ((pstCurrData->stDailyPrice.ulBegin == pstCurrData->stDailyPrice.ulEnd) &&
        (pstCurrData->stDailyPrice.ulHigh  == pstCurrData->stDailyPrice.ulLow)) return INVAILD_ULONG;
    if (pstCurrData->stDailyPrice.ulEnd < pstStockCtrl->ulWishPrice) return INVAILD_ULONG;

    return pstCurrData->stDailyPrice.ulEnd;
}

// if current price is between gain and loss price, sell price depends on every method
ULONG RISE_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
#if 1
    ULONG i;
    ULONG ulSellPrice;
    PRICE_S *pstMin30Price = pstCurrData->astMin30Price;
    
    if (RISE_HOLD_DAYS > GetDateInterval(pstStockCtrl->ulBuyDate, pstCurrData->ulDate)) {
        //ULONG ulHoldDays = GetDateInterval(pstStockCtrl->ulBuyDate, pstCurrData->ulDate);
        pstStockCtrl->ulLossPrice=(ULONG)(pstStockCtrl->ulLossPrice*1.01F);
        return INVAILD_ULONG;
    }

    for (i=0;i<8;i++, pstMin30Price++) {
        if (FILE_VAILD_PRICE != pstMin30Price->ulFlag) break;
    }

    if (i>1) {
        // sell at 14:30
        ulSellPrice = pstCurrData->astMin30Price[i-2].ulEnd;
    }
    else {
        // get a random sell price
        ulSellPrice = RandomUlong(pstCurrData->stDailyPrice.ulLow, pstCurrData->stDailyPrice.ulHigh);
    }

    return ulSellPrice;
#else
    return INVAILD_ULONG;
#endif
}

