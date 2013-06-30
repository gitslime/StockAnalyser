#include "../common/comm.h"
#include "../common/file.h"

#define SMA_MEAN_DAYS   (15)

ULONG SMA_GetMinIndex(VOID)
{
    return SMA_MEAN_DAYS;
}

#define SMA_MA_THRETHOLD        (1.01F)

BOOL_T SMA_Statistics(IN FILE_WHOLE_DATA_S *pstSettleData)
{
    ULONG ulPrevMa;
    ULONG ulCurrMa;
    FILE_WHOLE_DATA_S *pstPrev=pstSettleData-1;
    FILE_WHOLE_DATA_S *pstCurr=pstSettleData;

    ulPrevMa=GetMean(SMA_MEAN_DAYS,pstPrev,INVAILD_ULONG);
    ulCurrMa=GetMean(SMA_MEAN_DAYS,pstCurr,ulPrevMa);

    if (pstPrev->stDailyPrice.ulBegin > ulPrevMa) return BOOL_FALSE;
    if (pstPrev->stDailyPrice.ulEnd   < ulPrevMa) return BOOL_FALSE;
    if (pstCurr->stDailyPrice.ulBegin < ulCurrMa) return BOOL_FALSE;
    if (pstCurr->stDailyPrice.ulEnd   < ulCurrMa) return BOOL_FALSE;
    if (ulCurrMa < (ULONG)(ulPrevMa*SMA_MA_THRETHOLD)) return BOOL_FALSE;

    return BOOL_TRUE;
}

BOOL_T SMA_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    return BOOL_TRUE;
}

ULONG SMA_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    return 0;
}

ULONG SMA_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    return 0;
}

ULONG SMA_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    return 0;
}

ULONG SMA_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    return 0;
}

