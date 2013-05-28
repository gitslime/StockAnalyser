#include "../common/comm.h"
#include "method.h"

#define FUNC_DECLARATION(METHOD)    \
    extern VOID METHOD##_Statistics(IN ULONG ulIndex, IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstSettleData);   \
    extern BOOL_T METHOD##_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo); \
    extern ULONG METHOD##_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl); \
    extern ULONG METHOD##_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl); \
    extern ULONG METHOD##_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl);  \
    extern ULONG METHOD##_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl);


#define METHOD_RISE      (0x0001)
FUNC_DECLARATION(RISE)

ULONG GetMethod(IN CHAR* szMethod)
{
    ULONG ulMethod;
    
    if (0 == _stricmp(szMethod, "rise")) {
        ulMethod = METHOD_RISE;
    }
    else {
        printf("invaild method type\n");
        ulMethod = INVAILD_ULONG;
        exit(2);
    }
    return ulMethod;
}

#define GET_SIM_FUNC(pstFuncSet, METHOD)    \
{ \
    (pstFuncSet)->pfGetGainPrice = METHOD##_GetGainPrice;   \
    (pstFuncSet)->pfGetLossPrice = METHOD##_GetLossPrice;   \
    (pstFuncSet)->pfGetBuyPrice  = METHOD##_GetBuyPrice;    \
    (pstFuncSet)->pfGetSellPrice = METHOD##_GetSellPrice;   \
    (pstFuncSet)->pfDailyChoose  = METHOD##_Choose;         \
    (pstFuncSet)->pfStatistics   = METHOD##_Statistics;     \
}

VOID METHOD_GetFuncSet(IN ULONG ulMethod, OUT METHOD_FUNC_SET_S *pstFuncSet)
{
    switch (ulMethod) {
        case METHOD_RISE:
            GET_SIM_FUNC(pstFuncSet, RISE);
            break;
        default:
            assert(0);
    }

    return;
}



