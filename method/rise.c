#include "../common/comm.h"
#include "../common/file.h"

#define RISE_CONTINUOUS_DAYS        (3)
#define RISE_WATCH_DAYS             (1)

VOID RISE_Statistics(IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstBeginData, IN FILE_WHOLE_DATA_S *pstFirstData)
{
    ULONG i;
    BOOL_T bIsCont;
    FLOAT fPrevRise, fWatchRise, fWatchDrop, fWatchEnd;
    FILE_WHOLE_DATA_S *pstPrev = pstBeginData-1;
    FILE_WHOLE_DATA_S *pstBeforePrev = NULL;
    FILE_WHOLE_DATA_S *pstBase = NULL;
    FILE_WHOLE_DATA_S *pstWatch = NULL;

    // make sure array is not out of bound
    if ((pstBeginData-pstFirstData) < RISE_CONTINUOUS_DAYS) {
        pstPrev = pstFirstData+RISE_CONTINUOUS_DAYS;
    }
    pstBeforePrev = pstPrev-1;
    pstBase = pstPrev-RISE_CONTINUOUS_DAYS;
    pstWatch = pstPrev+1;
    ulEntryCnt -= pstWatch-pstBeginData;

    for (i=0;i<ulEntryCnt;i++, pstBase++, pstPrev++, pstWatch++, pstBeforePrev++) {
        bIsCont = GetTotalRise(RISE_CONTINUOUS_DAYS, pstPrev, RISE_TYPE_END, &fPrevRise);
        if (BOOL_FALSE == bIsCont) continue;

        GetTotalRise(RISE_WATCH_DAYS, pstWatch, RISE_TYPE_HIGH, &fWatchRise);
        GetTotalRise(RISE_WATCH_DAYS, pstWatch, RISE_TYPE_END, &fWatchEnd);
        GetTotalRise(RISE_WATCH_DAYS, pstWatch, RISE_TYPE_LOW, &fWatchDrop);

        printf("%d,%f,%f,%f,%f,%f\n", pstWatch->ulDate, fPrevRise, fWatchRise, fWatchDrop, fWatchEnd,
                            (FLOAT)pstPrev->stDailyPrice.ulVol/pstBeforePrev->stDailyPrice.ulVol);
    }

    return;
}

#define RISE_CHOOSE_DAYS        (2)
#define RISE_THRESHOLD_RATE     (1.10F)
#define RISE_THRESHOLD_DAILY    (1.02F)
#define RISE_BUY_HOUR           (14)

BOOL_T RISE_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    BOOL_T bIsContinuous;
    FLOAT  fPrevRise;
    FLOAT  fThreshPrice;
    FILE_WHOLE_DATA_S *pstBase = pstCurrData - RISE_CHOOSE_DAYS;

    // make sure array is not out of bound
    if (ulIndex <= RISE_CHOOSE_DAYS) return BOOL_FALSE;
    
    bIsContinuous = GetTotalRise(RISE_CHOOSE_DAYS, pstCurrData, RISE_TYPE_END, &fPrevRise);
    if (BOOL_FALSE == bIsContinuous) return BOOL_FALSE;

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

#define RISE_GAIN_THRESHOLD      (1.05F)
#define RISE_LOSS_THRESHOLD      (0.97F)

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
    ULONG i;
    ULONG ulSellPrice;
    PRICE_S *pstMin30Price = pstCurrData->astMin30Price;

    (VOID)pstStockCtrl;

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
}

