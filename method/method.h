#ifndef __METHOD_H__
#define __METHOD_H__

VOID RISE_Statistics(IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstBeginData, IN FILE_WHOLE_DATA_S *pstFirstData);
BOOL_T RISE_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo);
ULONG RISE_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl);
ULONG RISE_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl);
ULONG RISE_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl);
ULONG RISE_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl);

#endif 

