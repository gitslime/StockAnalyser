#include "../common/comm.h"
#include "method.h"

#define FUNC_DECLARATION(METHOD)    \
    extern BOOL_T METHOD##_Statistics(IN FILE_WHOLE_DATA_S *pstBuyEntry);   \
    extern BOOL_T METHOD##_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo); \
    extern ULONG METHOD##_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl); \
    extern ULONG METHOD##_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl); \
    extern ULONG METHOD##_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, IN ULONG ulWishPrice);  \
    extern ULONG METHOD##_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl); \
    extern ULONG METHOD##_SortWishList(IN ULONG ulWishCnt, IN SIM_STOCK_DATA_S *pstAllData, INOUT ULONG *aulWishList); \
    extern VOID METHOD##_SetParam(IN ULONG ulCnt, IN FLOAT *afParam);

#define METHOD_RISE      (0x0001)
#define METHOD_SMA       (0x0002)
#define METHOD_MMA       (0x0003)

FUNC_DECLARATION(RISE)
FUNC_DECLARATION(SMA)
FUNC_DECLARATION(MMA)

ULONG GetMethod(IN CHAR* szMethod)
{
    ULONG ulMethod;
    
    if (0 == _stricmp(szMethod, "rise")) {
        ulMethod = METHOD_RISE;
    }
    else if (0 == _stricmp(szMethod, "sma")) {
        ulMethod = METHOD_SMA;
    }
    else if (0 == _stricmp(szMethod, "mma")) {
        ulMethod = METHOD_MMA;
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
    (pstFuncSet)->pfSortWishList = METHOD##_SortWishList;   \
    (pstFuncSet)->pfSetParam     = METHOD##_SetParam;       \
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
        case METHOD_MMA:
            GET_SIM_FUNC(pstFuncSet, MMA);
            break;
        default:
            assert(0);
    }

    return;
}



