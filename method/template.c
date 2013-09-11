#include "../common/comm.h"
#include "../common/file.h"

VOID MMA_SetParam(IN ULONG ulCnt, IN FLOAT *afParam)
{
    return;
}

BOOL_T MMA_Statistics(IN FILE_WHOLE_DATA_S *pstSettleData)
{
    return BOOL_TRUE;
}

BOOL_T MMA_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    return BOOL_TRUE;
}

ULONG MMA_SortWishList(IN ULONG ulWishCnt, IN SIM_STOCK_DATA_S *pstAllData, INOUT SIM_WISH_LIST_S *astWishList)
{
    return 0;
}

ULONG MMA_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    return 0;
}

ULONG MMA_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    return 0;
}

ULONG MMA_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    return 0;
}

ULONG MMA_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    return 0;
}


