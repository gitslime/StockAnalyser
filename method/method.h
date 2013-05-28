#ifndef __METHOD_H__
#define __METHOD_H__

typedef ULONG (*GetGainPrice_PF)(IN FILE_WHOLE_DATA_S *, INOUT STOCK_CTRL_S *);
typedef ULONG (*GetLossPrice_PF)(IN FILE_WHOLE_DATA_S *, INOUT STOCK_CTRL_S *);
typedef ULONG (*GetBuyPrice_PF)(IN FILE_WHOLE_DATA_S *, INOUT STOCK_CTRL_S *);
typedef ULONG (*GetSellPrice_PF)(IN FILE_WHOLE_DATA_S *, INOUT STOCK_CTRL_S *);
typedef BOOL_T (*Choose_PF)(IN ULONG, IN FILE_WHOLE_DATA_S *, OUT CHOOSE_PRE_DEAL_S *);
typedef VOID (*Statistics_PF)(IN ULONG ulIndex, IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstSettleData);

typedef struct tagMethodFuncSet
{
    GetGainPrice_PF pfGetGainPrice;
    GetLossPrice_PF pfGetLossPrice;
    GetBuyPrice_PF  pfGetBuyPrice; 
    GetSellPrice_PF pfGetSellPrice;
    Choose_PF       pfDailyChoose;
    Statistics_PF   pfStatistics;
}METHOD_FUNC_SET_S; 

ULONG GetMethod(IN CHAR* szMethod);
VOID METHOD_GetFuncSet(IN ULONG ulMethod, OUT METHOD_FUNC_SET_S *pstFuncSet);

#endif 

