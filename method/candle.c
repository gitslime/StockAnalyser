#include "../common/comm.h"
#include "../common/file.h"

#define CANDLE_HOLD_DAYS              (1)     // days hold a stock
#define CANDLE_WATCH_DAYS             (3)     // days watching to judge whether to buy

VOID CANDLE_Statistics(IN ULONG ulIndex, IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstSettleData)
{
    ULONG i, j, ulStartIndex;
    ULONG ulWatchCandleType, ulCandleType;
    FLOAT fHoldRise;
    FILE_WHOLE_DATA_S *pstBuy   = NULL;
    FILE_WHOLE_DATA_S *pstBase  = NULL;
    FILE_WHOLE_DATA_S *pstTemp  = NULL;
    FILE_WHOLE_DATA_S *pstWatch = pstSettleData;

    // make sure array is not out of bound
    ulStartIndex = CANDLE_HOLD_DAYS+CANDLE_WATCH_DAYS;
    if (ulIndex < ulStartIndex) {
        pstWatch+=(ulStartIndex-ulIndex);
        ulEntryCnt-=(ulStartIndex-ulIndex);
        if (ulEntryCnt > 0xFF000000) return;
    }
    pstBuy  = pstWatch-CANDLE_HOLD_DAYS;
    pstBase = pstBuy-CANDLE_WATCH_DAYS;

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

BOOL_T CANDLE_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
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

ULONG CANDLE_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    assert(pstCurrData->ulDate == pstStockCtrl->ulBuyDate);

    return (ULONG)(pstCurrData->stDailyPrice.ulEnd * RISE_GAIN_THRESHOLD);
}

ULONG CANDLE_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    assert(pstCurrData->ulDate == pstStockCtrl->ulBuyDate);

    return (ULONG)(pstCurrData->stDailyPrice.ulEnd * RISE_LOSS_THRESHOLD);
}

ULONG CANDLE_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
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
ULONG CANDLE_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
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


