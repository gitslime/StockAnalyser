#include "../common/comm.h"
#include "../common/file.h"

#define MMA_BUY_HOUR (14)

typedef struct tagMmaParam
{
    ULONG ulMaShort;
    ULONG ulMaLong;
}MMA_PARAM_S;

MMA_PARAM_S g_stMmaParam;

#define MMA_PARAM_LONG  (g_stMmaParam.ulMaLong)
#define MMA_PARAM_SHORT (g_stMmaParam.ulMaShort)
#define MMA_GET_CROSS_PRICE(CurrMaShort, CurrMaLong, ShortPrice, LongPrice) \
    (((((FLOAT)(CurrMaLong)) - ((FLOAT)(CurrMaShort))) * MMA_PARAM_SHORT * MMA_PARAM_LONG)  + \
     ((((FLOAT)(ShortPrice)) * MMA_PARAM_LONG) - (((FLOAT)(LongPrice)) * MMA_PARAM_SHORT))) / \
     (MMA_PARAM_LONG - MMA_PARAM_SHORT)

VOID MMA_SetParam(IN ULONG ulCnt, IN FLOAT *afParam)
{
    assert(ulCnt == sizeof(MMA_PARAM_S)/sizeof(ULONG));

    g_stMmaParam.ulMaShort = (ULONG)afParam[0];
    g_stMmaParam.ulMaLong  = (ULONG)afParam[1];

    assert(g_stMmaParam.ulMaLong > g_stMmaParam.ulMaShort);
    
    return;
}

BOOL_T MMA_Statistics(IN FILE_WHOLE_DATA_S *pstSettleData)
{
    return BOOL_TRUE;
}

BOOL_T MMA_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    ULONG ulCurrMaShort, ulCurrMaLong;
    FLOAT fThreshPrice;
    INT iMafdShort, iMafdLong;
    FILE_WHOLE_DATA_S *pstChoose=pstCurrData;
    FILE_WHOLE_DATA_S *pstPrevShort=pstChoose-MMA_PARAM_SHORT+1;
    FILE_WHOLE_DATA_S *pstPrevLong =pstChoose-MMA_PARAM_LONG+1;
    
    if (ulIndex < MMA_PARAM_LONG) return BOOL_FALSE;

    ulCurrMaShort = GetMeanBackward(MMA_PARAM_SHORT, pstChoose, INVAILD_ULONG);
    ulCurrMaLong  = GetMeanBackward(MMA_PARAM_LONG,  pstChoose, INVAILD_ULONG);

    // short cross long to buy
    if (ulCurrMaShort > ulCurrMaLong) return BOOL_FALSE;

    // estimate buy price
    fThreshPrice = MMA_GET_CROSS_PRICE(ulCurrMaShort, ulCurrMaLong, 
                                       pstPrevShort->stDailyPrice.ulEnd, 
                                       pstPrevLong->stDailyPrice.ulEnd);
    if (fThreshPrice > (pstChoose->stDailyPrice.ulEnd * (1+STOCK_RISE_THREASHOLD))) return BOOL_FALSE;

    // get weight
    iMafdShort = GetMeanFdBackward(MMA_PARAM_SHORT,pstChoose);
    iMafdLong  = GetMeanFdBackward(MMA_PARAM_LONG, pstChoose);

    // set deal info   
    pstDealInfo->bIsSell    = BOOL_FALSE;
    pstDealInfo->usDealHour = MMA_BUY_HOUR;
    pstDealInfo->usDealMin  = (USHORT)RandomUlong(50,60);
    pstDealInfo->bIsHigher  = BOOL_TRUE;    
    pstDealInfo->fWeight    = (FLOAT)(iMafdShort - iMafdLong);
    pstDealInfo->fThresholdPrice = FILE_PRICE2REAL(MAX(fThreshPrice, ulCurrMaLong));

    return BOOL_TRUE;
}

ULONG MMA_SortWishList(IN ULONG ulWishCnt, IN SIM_STOCK_DATA_S *pstAllData, INOUT SIM_WISH_LIST_S *astWishList)
{
    return ulWishCnt;
}

ULONG MMA_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    return INVAILD_ULONG;
}

ULONG MMA_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    ULONG ulCurrMaShort, ulCurrMaLong;
    FLOAT fThreshPrice;
    FILE_WHOLE_DATA_S *pstPrevShort=pstCurrData-MMA_PARAM_SHORT+1;
    FILE_WHOLE_DATA_S *pstPrevLong =pstCurrData-MMA_PARAM_LONG+1;

    ulCurrMaShort = GetMean(MMA_PARAM_SHORT, pstCurrData, INVAILD_ULONG);
    ulCurrMaLong  = GetMean(MMA_PARAM_LONG,  pstCurrData, INVAILD_ULONG);
#if 0
    // estimate corss price
    fThreshPrice = MMA_GET_CROSS_PRICE(ulCurrMaShort, ulCurrMaLong, 
                                       pstPrevShort->stDailyPrice.ulEnd, 
                                       pstPrevLong->stDailyPrice.ulEnd);

    return (fThreshPrice <= 0) ? 0 : (ULONG)(fThreshPrice * 0.96);
#endif
    return (ulCurrMaShort < ulCurrMaLong) ? INVAILD_ULONG : 0;
}

ULONG MMA_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, IN ULONG ulWishPrice)
{
    FLOAT fRise;

    // get today's rise
    (VOID)GetTotalRise(1,pstCurrData,RISE_TYPE_END,&fRise);

    if (fRise > STOCK_RISE_THREASHOLD) return INVAILD_ULONG;   // limit up cann't buy

    // this check is for ST limit up
    if ((pstCurrData->stDailyPrice.ulBegin == pstCurrData->stDailyPrice.ulEnd) &&
        (pstCurrData->stDailyPrice.ulHigh  == pstCurrData->stDailyPrice.ulLow)) return INVAILD_ULONG;

    // check wish price
    if (pstCurrData->stDailyPrice.ulEnd < ulWishPrice) return INVAILD_ULONG;

    return pstCurrData->stDailyPrice.ulEnd;
}

ULONG MMA_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    // not sell until reachs threshold
    return INVAILD_ULONG;
}

