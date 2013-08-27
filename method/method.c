#include "../common/comm.h"
#include "method.h"

#define FUNC_DECLARATION(METHOD)    \
    extern BOOL_T METHOD##_Statistics(IN FILE_WHOLE_DATA_S *pstBuyEntry);   \
    extern BOOL_T METHOD##_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo); \
    extern ULONG METHOD##_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl); \
    extern ULONG METHOD##_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl); \
    extern ULONG METHOD##_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, IN ULONG ulWishPrice);  \
    extern ULONG METHOD##_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl); \
    extern ULONG METHOD##_GetMinIndex(VOID); \
    extern VOID METHOD##_SortWishList(IN ULONG ulWishCnt, INOUT ULONG *aulWishList);

#define METHOD_RISE      (0x0001)
#define METHOD_SMA       (0x0002)

FUNC_DECLARATION(RISE)
FUNC_DECLARATION(SMA)

ULONG GetMethod(IN CHAR* szMethod)
{
    ULONG ulMethod;
    
    if (0 == _stricmp(szMethod, "rise")) {
        ulMethod = METHOD_RISE;
    }
    else if (0 == _stricmp(szMethod, "sma")) {
        ulMethod = METHOD_SMA;
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
    (pstFuncSet)->pfGetMinIndex  = METHOD##_GetMinIndex;    \
    (pstFuncSet)->pfSortWishList = METHOD##_SortWishList;   \
}

VOID METHOD_GetFuncSet(IN ULONG ulMethod, OUT METHOD_FUNC_SET_S *pstFuncSet)
{
    switch (ulMethod) {
        case METHOD_RISE:
            GET_SIM_FUNC(pstFuncSet, RISE);
            break;
        case METHOD_SMA:
            GET_SIM_FUNC(pstFuncSet, SMA);
            break;
        default:
            assert(0);
    }

    return;
}



