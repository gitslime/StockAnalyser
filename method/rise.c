#include "../common/comm.h"
#include "../common/file.h"

#define RISE_HOLD_DAYS              (1)     // days hold a stock
#define RISE_WATCH_DAYS             (3)     // days watching to judge whether to buy
#define RISE_MEAN_DAYS              (15)
#define RISE_HOLD_THRESHOLD         (0.093F)

VOID RISE_Statistics(IN ULONG ulIndex, IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstSettleData)
{
    ULONG i, j, ulStartIndex;
    ULONG ulWatchCandleType, ulCandleType;
    FLOAT fHoldRise;
    FILE_WHOLE_DATA_S *pstBuy   = NULL;
    FILE_WHOLE_DATA_S *pstBase  = NULL;
    FILE_WHOLE_DATA_S *pstTemp  = NULL;
    FILE_WHOLE_DATA_S *pstWatch = pstSettleData;

    // make sure array is not out of bound
    ulStartIndex = RISE_HOLD_DAYS+RISE_WATCH_DAYS;
    if (ulIndex < ulStartIndex) {
        pstWatch+=(ulStartIndex-ulIndex);
        ulEntryCnt-=(ulStartIndex-ulIndex);
        if (ulEntryCnt > 0xFF000000) return;
    }
    pstBuy  = pstWatch-RISE_HOLD_DAYS;
    pstBase = pstBuy-RISE_WATCH_DAYS;

    for (i=0;i<ulEntryCnt;i++, pstBase++, pstBuy++, pstWatch++) {
        (VOID)GetTotalRise(RISE_HOLD_DAYS, pstWatch, RISE_TYPE_END, &fHoldRise);
        if (fHoldRise < RISE_HOLD_THRESHOLD) continue;

        ulWatchCandleType=0;
        for (j=RISE_WATCH_DAYS,pstTemp=pstBuy;j>0;j--,pstTemp--) {
            ulCandleType=GetCandleType(pstTemp);
            ulWatchCandleType = ulWatchCandleType << 8;
            ulWatchCandleType = ulWatchCandleType | ulCandleType;
        }

        printf("%d,%u,%u,%u,%u,%f\n", pstBuy->ulDate, 
            ulWatchCandleType&0xFF, (ulWatchCandleType&0xFF00)>>8, (ulWatchCandleType&0xFF0000)>>16, 
            ulWatchCandleType, fHoldRise);
    }

    return;
}

#define RISE_CHOOSE_DAYS        (RISE_WATCH_DAYS-1)
#define RISE_THRESHOLD_RATE     (1.10F)
#define RISE_THRESHOLD_DAILY    (1.02F)
#define RISE_BUY_HOUR           (14)
#if 0
BOOL_T RISE_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    BOOL_T bIsContinuous;
    FLOAT  fPrevRise;
    FLOAT  fThreshPrice;
    ULONG  ulMeanPrice=INVAILD_ULONG;
    FILE_WHOLE_DATA_S *pstBase = pstCurrData - RISE_CHOOSE_DAYS;
    FILE_WHOLE_DATA_S *pstTemp = pstBase-1;

    // make sure array is not out of bound
    if (ulIndex <= (RISE_CHOOSE_DAYS+RISE_MEAN_DAYS)) return BOOL_FALSE;

    if (ulIndex>(RISE_CHOOSE_DAYS+1)) {
        (VOID)GetTotalRise(1, pstBase, RISE_TYPE_END, &fPrevRise);
        if (fPrevRise > 0) return BOOL_FALSE;
    }
    
    ulMeanPrice = GetMean(RISE_MEAN_DAYS, pstBase, ulMeanPrice);
    if (pstBase->stDailyPrice.ulEnd>ulMeanPrice) return BOOL_FALSE;

    #if 1
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
    
    bIsContinuous = GetTotalRise(RISE_CHOOSE_DAYS, pstCurrData, RISE_TYPE_END, &fPrevRise);
    if (BOOL_FALSE == bIsContinuous) return BOOL_FALSE;

    if (fPrevRise > 0.15) return BOOL_FALSE;

    pstDealInfo->bIsSell    = BOOL_FALSE;
    pstDealInfo->usDealHour = RISE_BUY_HOUR;
    pstDealInfo->usDealMin  = (USHORT)RandomUlong(50,60);
    pstDealInfo->bIsHigher  = BOOL_TRUE;

    // make sure rising continuously
    fThreshPrice = MAX((RISE_THRESHOLD_RATE  * pstBase->stDailyPrice.ulEnd),
                       (RISE_THRESHOLD_DAILY * pstCurrData->stDailyPrice.ulEnd));
    
    pstDealInfo->fThresholdPrice = FILE_PRICE2REAL(fThreshPrice);

    return BOOL_TRUE;
}
#endif
BOOL_T RISE_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    FLOAT fThreshPrice;
    ULONG ulCandleType;
    FILE_WHOLE_DATA_S *pstPrev = pstCurrData-1;

    if (ulIndex<RISE_CHOOSE_DAYS) return BOOL_FALSE;

    ulCandleType=GetCandleType(pstCurrData);
    if ((CANDLE_TYPE_BARE_LARGE_GAIN != ulCandleType) &&
        (CANDLE_TYPE_LARGE_LOSS != ulCandleType) &&
        (CANDLE_TYPE_LARGE_GAIN != ulCandleType))
        return BOOL_FALSE;

    ulCandleType=GetCandleType(pstPrev);
    if ((CANDLE_TYPE_BARE_LARGE_GAIN != ulCandleType) &&
        (CANDLE_TYPE_LARGE_GAIN != ulCandleType))
        return BOOL_FALSE;

    pstDealInfo->bIsSell    = BOOL_FALSE;
    pstDealInfo->usDealHour = RISE_BUY_HOUR;
    pstDealInfo->usDealMin  = (USHORT)RandomUlong(50,60);
    pstDealInfo->bIsHigher  = BOOL_TRUE;

    fThreshPrice = 1.05F * pstCurrData->stDailyPrice.ulEnd;
    pstDealInfo->fThresholdPrice = FILE_PRICE2REAL(fThreshPrice);

    return BOOL_TRUE;
}

#define RISE_GAIN_THRESHOLD      (1.08F) 
#define RISE_LOSS_THRESHOLD      (0.89F)

ULONG RISE_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    assert(pstCurrData->ulDate == pstStockCtrl->ulBuyDate);

    return (ULONG)(pstCurrData->stDailyPrice.ulEnd * RISE_GAIN_THRESHOLD);
}

ULONG RISE_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    assert(pstCurrData->ulDate == pstStockCtrl->ulBuyDate);

    return (ULONG)(pstCurrData->stDailyPrice.ulEnd * RISE_LOSS_THRESHOLD);
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

