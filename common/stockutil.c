#include "comm.h"
#include "file.h"

// get total rise of n days, including current
BOOL_T GetTotalRise(IN ULONG ulCnt, IN FILE_WHOLE_DATA_S *pstCurrent, IN ULONG ulType, OUT FLOAT *pfTotalRise)
{
    ULONG i;
    BOOL_T bIsContinuous = BOOL_TRUE;
    ULONG ulBaseEndPrice;
    ULONG ulHigh, ulLow;
    FLOAT fPrevPrice, fCurrPrice;
    FLOAT fMulti, fAdder;
    FILE_WHOLE_DATA_S *pstBase = pstCurrent - ulCnt;
    FILE_WHOLE_DATA_S *pstTemp = pstBase+1;

    assert(FILE_VAILD_PRICE == pstBase->stDailyPrice.ulFlag);
    ulBaseEndPrice = pstBase->stDailyPrice.ulEnd;

    fPrevPrice = (FLOAT)ulBaseEndPrice;
    fMulti = 1;
    fAdder = 0;
    ulHigh = pstTemp->stDailyPrice.ulHigh;
    ulLow  = pstTemp->stDailyPrice.ulLow;
    for (i=0;i<ulCnt;i++) {
        if (FILE_VAILD_FACTOR == pstTemp->stFactor.ulFlag) {
            fMulti *= pstTemp->stFactor.fMulti;
            fAdder += pstTemp->stFactor.fAdder;
        }
        
        if (RISE_TYPE_END == ulType) {
            fCurrPrice = fMulti * pstTemp->stDailyPrice.ulEnd + fAdder;
        }
        else if (RISE_TYPE_HIGH == ulType) {
            ulHigh = MAX(pstTemp->stDailyPrice.ulHigh, ulHigh);
            fCurrPrice = fMulti * ulHigh + fAdder;
        }
        else if (RISE_TYPE_LOW == ulType) {
            ulLow = MIN(pstTemp->stDailyPrice.ulHigh, ulLow);
            fCurrPrice = fMulti * ulLow + fAdder;
        }
        else if (RISE_TYPE_BEGIN == ulType) {
            fCurrPrice = fMulti * pstTemp->stDailyPrice.ulBegin + fAdder;
        }
        else {
            assert(0);
        }
        if (fCurrPrice <= 1.02*fPrevPrice) bIsContinuous = BOOL_FALSE;

        pstTemp++;
        fPrevPrice = fCurrPrice;
    }

    *pfTotalRise = fCurrPrice / ulBaseEndPrice - 1;

    return bIsContinuous;
}

VOID GetFactor(IN ULONG ulStartDate, IN FILE_WHOLE_DATA_S *pstCurrData,
               OUT FLOAT *pfMulti, OUT FLOAT *pfAdder)
{
    FLOAT fMulti, fAdder;
    FILE_WHOLE_DATA_S *pstTemp = pstCurrData;

    fMulti = 1;
    fAdder = 0;
    while (1) {
        if (pstTemp->ulDate <= ulStartDate) break;
        
        if (FILE_VAILD_FACTOR == pstTemp->stFactor.ulFlag) {
            fMulti *= pstTemp->stFactor.fMulti;
            fAdder += pstTemp->stFactor.fAdder;
        }
        pstTemp--;
    }

    *pfMulti = fMulti;
    *pfAdder = fAdder;
    
    return;
}

ULONG GetMean(IN ULONG ulDays, IN FILE_WHOLE_DATA_S *pstData, IN ULONG ulPrevMean)
{
    ULONG ulSum=0;
    
    if (INVAILD_ULONG == ulPrevMean) {
        ULONG i;

        for (i=0;i<ulDays;i++) {
            ulSum+=pstData->stDailyPrice.ulEnd;
            pstData--;
        }
    }
    else {
        ulSum=ulDays*ulPrevMean;
        ulSum+=pstData->stDailyPrice.ulEnd;
        pstData-=ulDays;
        ulSum-=pstData->stDailyPrice.ulEnd;
    }
   
    return (ULONG)((FLOAT)ulSum/ulDays);
}

ULONG GetMeanBackward(IN ULONG ulDays, IN FILE_WHOLE_DATA_S *pstData, IN ULONG ulPrevMean)
{
    FLOAT fSum=0;
    FLOAT fMulti = 1;
    FLOAT fAdder = 0;

    if (INVAILD_ULONG == ulPrevMean) {
        ULONG i;

        for (i=0;i<ulDays;i++) {
            if (FILE_VAILD_FACTOR == pstData->stFactor.ulFlag) {
                fMulti *= pstData->stFactor.fMulti;
                fAdder += pstData->stFactor.fAdder;
            }

            fSum += pstData->stDailyPrice.ulEnd/fMulti - fAdder;
            pstData--;
        }
    }
    else {
        if (FILE_VAILD_FACTOR == pstData->stFactor.ulFlag) {
            fMulti *= pstData->stFactor.fMulti;
            fAdder += pstData->stFactor.fAdder;
        }

        fSum=(FLOAT)ulDays*ulPrevMean;
        fSum+=pstData->stDailyPrice.ulEnd/fMulti - fAdder;
        pstData-=ulDays;
        fSum-=pstData->stDailyPrice.ulEnd;
        
    }

    return (ULONG)(fSum/ulDays);
}

// get First derivative of move average. first derivative = current ma - previous ma
INT GetMeanFdBackward(IN ULONG ulDays, IN FILE_WHOLE_DATA_S *pstData)
{
    ULONG ulPrevMa=GetMeanBackward(ulDays, pstData-1, INVAILD_ULONG);
    ULONG ulCurrMa=GetMeanBackward(ulDays, pstData, ulPrevMa);

    return (((INT)ulCurrMa)-((INT)ulPrevMa));
}

INT GetMeanSdBackward(IN ULONG ulDays, IN FILE_WHOLE_DATA_S *pstData)
{
    INT iPPrevMa = (INT)GetMeanBackward(ulDays, pstData-2, INVAILD_ULONG);
    INT iPrevMa  = (INT)GetMeanBackward(ulDays, pstData-1, iPPrevMa);
    INT iCurrMa  = (INT)GetMeanBackward(ulDays, pstData, iPrevMa);

    return ((iCurrMa-iPrevMa)-(iPrevMa-iPPrevMa));
}

FLOAT GetVolRatio(IN FILE_WHOLE_DATA_S *pstData)
{
    FILE_WHOLE_DATA_S *pstPrev = pstData-1;

    return ((FLOAT)pstData->stDailyPrice.ulVol)/((FLOAT)pstPrev->stDailyPrice.ulVol);
}

ULONG GetBuyCnt(IN ULONG ulHandLimit, IN ULONG ulBuyPrice)
{
    ULONG ulHand=ulHandLimit/ulBuyPrice/100;
    ULONG ulBuyCap=(ulHand*ulBuyPrice*100);

    if ((ulBuyCap<(ulHandLimit*0.9)) || (ulBuyCap>(ulHandLimit*1.1))) return 0;

    return ulHand*100;
}

ULONG GetCandleType(IN FILE_WHOLE_DATA_S *pstDaily)
{
#define SOILD_TYPE_CLOSE(fRate)         ((-0.004F<=(fRate))&&((fRate)<=0.004F))
#define SOILD_TYPE_LITTLE(fRate)        ((-0.020F<=(fRate))&&((fRate)<=0.020F))
#define SOILD_TYPE_MIDDLE(fRate)        ((-0.050F<=(fRate))&&((fRate)<=0.050F))
#define PRICE_GET_RATE(ulPrice, ulBasePrice)    (((FLOAT)(ulPrice)-(FLOAT)(ulBasePrice))/(FLOAT)(ulBasePrice))


    ULONG ulCandleType;
    ULONG ulBegin = pstDaily->stDailyPrice.ulBegin;
    ULONG ulEnd   = pstDaily->stDailyPrice.ulEnd;
    ULONG ulHigh  = pstDaily->stDailyPrice.ulHigh;
    ULONG ulLow   = pstDaily->stDailyPrice.ulLow;
    ULONG ulCLow  = MIN(ulBegin, ulEnd);
    ULONG ulCHigh = MAX(ulBegin, ulEnd);

    FLOAT fSoildRate = PRICE_GET_RATE(ulBegin, ulEnd);
    FLOAT fLowRate   = PRICE_GET_RATE(ulCLow, ulLow); 
    FLOAT fHighRate  = PRICE_GET_RATE(ulCHigh, ulHigh);
    FLOAT fTotalRate = PRICE_GET_RATE(ulHigh, ulLow);

    if (SOILD_TYPE_CLOSE(fSoildRate)) {
        if (SOILD_TYPE_CLOSE(fTotalRate)) {
            ulCandleType = CANDLE_TYPE_LINE;
        }
        else if (SOILD_TYPE_CLOSE(fLowRate)) {
            ulCandleType = CANDLE_TYPE_T_INVERSE;
        }
        else if (SOILD_TYPE_CLOSE(fHighRate)) {
            ulCandleType = CANDLE_TYPE_T_RIGHT;
        }
        else if (SOILD_TYPE_LITTLE(fTotalRate)) {
            ulCandleType = CANDLE_TYPE_CROSS;
        }
        else {
            ulCandleType = CANDLE_TYPE_CROSS_LONG;
        }
    }
    else if (SOILD_TYPE_LITTLE(fSoildRate)) {
        if (fHighRate<(-4*fLowRate)) {
            ulCandleType = CANDLE_TYPE_HAMMER_INVERSE;
        }
        else if (fLowRate>(-4*fHighRate)) {
            ulCandleType = CANDLE_TYPE_HAMMER_RIGHT;
        }
        else if (ulBegin > ulEnd) {
            ulCandleType = CANDLE_TYPE_LITTLE_LOSS;
        }
        else {
            ulCandleType = CANDLE_TYPE_LITTLE_GAIN;
        }
    }
    else if (SOILD_TYPE_MIDDLE(fSoildRate)) {
        if (SOILD_TYPE_CLOSE(fHighRate)) {
            ulCandleType = (ulBegin > ulEnd) ? CANDLE_TYPE_BARE_HEAD_LOSS : CANDLE_TYPE_BARE_HEAD_GAIN;
        }
        else if (SOILD_TYPE_CLOSE(fLowRate)) {
            ulCandleType = (ulBegin > ulEnd) ? CANDLE_TYPE_BARE_FEET_LOSS : CANDLE_TYPE_BARE_FEET_GAIN;
        }
        else {
            ulCandleType = (ulBegin > ulEnd) ? CANDLE_TYPE_MIDDLE_LOSS : CANDLE_TYPE_MIDDLE_GAIN;
        }
    }
    else {
        if ((SOILD_TYPE_CLOSE(fHighRate)) && (SOILD_TYPE_CLOSE(fLowRate))) {
            ulCandleType = (ulBegin > ulEnd) ? CANDLE_TYPE_BARE_LARGE_LOSS: CANDLE_TYPE_BARE_LARGE_GAIN;
        }
        else {
            ulCandleType = (ulBegin > ulEnd) ? CANDLE_TYPE_LARGE_LOSS : CANDLE_TYPE_LARGE_GAIN;
        }
    }

    return ulCandleType;
}

ULONG GetCodeByIndex(IN CHAR *szIndex)
{
    if (0 == _stricmp(szIndex, "1A0001")) return INDEX_CODE_SH;
    if (0 == _stricmp(szIndex, "399001")) return INDEX_CODE_SZ;
    if (0 == _stricmp(szIndex, "399005")) return INDEX_CODE_ZXB;
    if (0 == _stricmp(szIndex, "399006")) return INDEX_CODE_CYB;

    return (ULONG)atol(szIndex);
}

