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
#define RISE_THRESHOLD_RATE     (1.1F)
#define RISE_THRESHOLD_DAILY    (1.02F)
#define RISE_BUY_HOUR           (14)
#define RISE_BUY_MIN            (50)

BOOL_T RISE_Choose(IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    BOOL_T bIsContinuous;
    FLOAT  fPrevRise;
    FLOAT  fThreshPrice;
    FILE_WHOLE_DATA_S *pstBase = pstCurrData - RISE_CHOOSE_DAYS;
    
    bIsContinuous = GetTotalRise(RISE_CHOOSE_DAYS, pstCurrData, RISE_TYPE_END, &fPrevRise);
    if (BOOL_FALSE == bIsContinuous) return BOOL_FALSE;

    pstDealInfo->bIsSell    = BOOL_FALSE;
    pstDealInfo->usDealHour = RISE_BUY_HOUR;
    pstDealInfo->usDealMin  = RISE_BUY_MIN;
    pstDealInfo->bIsHigher  = BOOL_TRUE;

    // make sure rising continuously
    fThreshPrice = MAX((RISE_THRESHOLD_RATE  * pstBase->stDailyPrice.ulEnd),
                       (RISE_THRESHOLD_DAILY * pstCurrData->stDailyPrice.ulEnd));
    
    pstDealInfo->fThresholdPrice = FILE_PRICE2REAL(fThreshPrice);

    return BOOL_TRUE;
}


