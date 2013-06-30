#include "../common/comm.h"
#include "../common/file.h"

#define SMA_MEAN_DAYS   (15)
#define SMA_CHECK_DAYS  (30)
#define SMA_CHECK_THRETHOLD (-0.1800F)

ULONG SMA_GetMinIndex(VOID)
{
    return MAX(SMA_MEAN_DAYS,SMA_CHECK_DAYS);
}

#define SMA_MA_THRETHOLD        (1.000F)

BOOL_T SMA_Statistics(IN FILE_WHOLE_DATA_S *pstBuyData)
{
    ULONG ulPrevMa, ulBaseMa, ulChooseMa;
    FILE_WHOLE_DATA_S *pstChoose=pstBuyData-1;
    FILE_WHOLE_DATA_S *pstBase=pstBuyData-2;
    FILE_WHOLE_DATA_S *pstPrev=pstBase-1;

    ulPrevMa=GetMean(SMA_MEAN_DAYS,pstPrev,INVAILD_ULONG);
    ulBaseMa=GetMean(SMA_MEAN_DAYS,pstBase,ulPrevMa);
    ulChooseMa=GetMean(SMA_MEAN_DAYS,pstChoose,ulBaseMa);

    printf("%f,%f,", GET_RATE(ulBaseMa, ulPrevMa), GET_RATE(ulChooseMa,ulBaseMa));

    return BOOL_TRUE;
}

#define SMA_BUY_HOUR           (14)

BOOL_T SMA_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    ULONG ulPrevMa,ulCurrMa;
    ULONG ulThreshPrice;
    ULONG i;
    FLOAT fPrevVolRatio, fChooseVolRatio;
    FILE_WHOLE_DATA_S *pstTemp=pstCurrData;
    FILE_WHOLE_DATA_S *pstChoose=pstCurrData;
    FILE_WHOLE_DATA_S *pstPrev=pstChoose-1;

    if (ulIndex <= SMA_GetMinIndex()) return BOOL_FALSE;

    // check factor
    for (i=SMA_MEAN_DAYS;i>0;i--,pstTemp--) {
        if (FILE_VAILD_FACTOR==pstTemp->stFactor.ulFlag) return BOOL_FALSE;
    }
#if 1
    // check volume ratio
    fPrevVolRatio=GetVolRatio(pstPrev);
    if ((fPrevVolRatio<1) || (fPrevVolRatio>2)) return BOOL_FALSE;

    fChooseVolRatio=GetVolRatio(pstChoose);
    if (fChooseVolRatio>2) return BOOL_FALSE;
#endif
    ulPrevMa=GetMean(SMA_MEAN_DAYS,pstPrev,INVAILD_ULONG);
    ulCurrMa=GetMean(SMA_MEAN_DAYS,pstChoose,ulPrevMa);

    // previous data across ma
    if (pstPrev->stDailyPrice.ulBegin > (ulPrevMa*0.99)) return BOOL_FALSE;
    if (pstPrev->stDailyPrice.ulEnd   < (ulPrevMa*1.01)) return BOOL_FALSE;

    // choose data above ma and previous price
    if (pstChoose->stDailyPrice.ulLow < ulCurrMa) return BOOL_FALSE;
    //if (pstChoose->stDailyPrice.ulBegin < ulCurrMa) return BOOL_FALSE;
    //if (pstChoose->stDailyPrice.ulEnd   < ulCurrMa) return BOOL_FALSE;
    //if (pstChoose->stDailyPrice.ulEnd < pstPrev->stDailyPrice.ulLow) return BOOL_FALSE;

    #if 1
    // ma should not rise too fast
    if (ulCurrMa > (ULONG)(ulPrevMa*SMA_MA_THRETHOLD)) return BOOL_FALSE;
    //#else 
    {
        FLOAT fDrop;
        GetTotalRise(SMA_CHECK_DAYS,pstPrev,RISE_TYPE_END,&fDrop);

        if (fDrop>SMA_CHECK_THRETHOLD) return BOOL_FALSE;

    }
    #endif

    // estimate buy price
    ulThreshPrice = MIN(ulCurrMa,ulPrevMa);
    
    pstDealInfo->bIsSell    = BOOL_FALSE;
    pstDealInfo->usDealHour = SMA_BUY_HOUR;
    pstDealInfo->usDealMin  = (USHORT)RandomUlong(50,60);
    pstDealInfo->bIsHigher  = BOOL_TRUE;
    pstDealInfo->fThresholdPrice = FILE_PRICE2REAL(ulThreshPrice);

    return BOOL_TRUE;
}

#define SMA_HOLD_DAYS           (5)            // days hold a stock
#define SMA_GAIN_THRESHOLD      (1.203F) 
#define SMA_GAIN_STEP           (0.010F)
#define SMA_LOSS_THRESHOLD      (0.883F)
#define SMA_LOSS_STEP           (0.020F)

ULONG SMA_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    ULONG i, ulHoldDays=0;
    ULONG ulPrevMa, ulCurrMa;
    ULONG ulBelowCnt;
    BOOL_T bIsSame=BOOL_FALSE;
    FILE_WHOLE_DATA_S *pstData=pstCurrData;

    assert(pstCurrData->ulDate>=pstStockCtrl->ulBuyDate);

    // point to buy date
    while (1) {
        ulHoldDays++;
        if (pstData->ulDate==pstStockCtrl->ulBuyDate) break;
        pstData--;
    }

    // get times of price below ma
    for(i=0,ulPrevMa=INVAILD_ULONG,ulBelowCnt=0;i<ulHoldDays;i++,pstData++) {
        ulCurrMa=GetMean(SMA_MEAN_DAYS,pstData,ulPrevMa);

        //if ((pstData->stDailyPrice.ulBegin < ulCurrMa) && (pstData->stDailyPrice.ulEnd < ulCurrMa)) {
        if (pstData->stDailyPrice.ulLow < ulCurrMa) {
            ulBelowCnt++;
        }

        ulPrevMa=ulCurrMa;
    }

    return (ULONG)(ulCurrMa * (SMA_GAIN_THRESHOLD-(ulBelowCnt*SMA_GAIN_STEP)));
}

ULONG SMA_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    ULONG i, ulHoldDays=0;
    ULONG ulPrevMa, ulCurrMa;
    ULONG ulPrevLow, ulMaDrift;
    ULONG ulBelowCnt;
    BOOL_T bIsSame=BOOL_FALSE;
    FILE_WHOLE_DATA_S *pstData=pstCurrData;

    assert(pstCurrData->ulDate>=pstStockCtrl->ulBuyDate);

    // point to buy date
    while (1) {
        ulHoldDays++;
        if (pstData->ulDate==pstStockCtrl->ulBuyDate) break;
        pstData--;
    }

    // get low point before buy
    for (i=SMA_MEAN_DAYS,ulPrevLow=pstData->stDailyPrice.ulLow;i>0;i--,pstData--) {
        ulPrevLow=MIN(ulPrevLow,pstData->stDailyPrice.ulLow);
    }
    pstData+=SMA_MEAN_DAYS;

    // get times of price below ma
    for(i=0,ulPrevMa=INVAILD_ULONG,ulBelowCnt=0;i<ulHoldDays;i++,pstData++) {
        ulCurrMa=GetMean(SMA_MEAN_DAYS,pstData,ulPrevMa);

        if ((pstData->stDailyPrice.ulBegin < ulCurrMa) && (pstData->stDailyPrice.ulEnd < ulCurrMa)) {
            ulBelowCnt++;
        }

        ulPrevMa=ulCurrMa;
    }
    ulMaDrift=(ULONG)(ulCurrMa * (SMA_LOSS_THRESHOLD+(ulBelowCnt*SMA_LOSS_STEP)));

    return MAX(ulPrevLow,ulMaDrift);
}

ULONG SMA_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
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

ULONG SMA_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    // not sell until reachs threshold
    return INVAILD_ULONG;
}

