#include "../common/comm.h"
#include "../common/file.h"

typedef struct tagOmaParam
{
    UINT32 ulMaDays;             
    FLOAT fNoThreshold;     /* negative offset */
    FLOAT fPoThreshold;     /* positive offset */
    UINT32 ulChkSellDays;
}OMA_PARAM_S;

static OMA_PARAM_S g_stOmaParam;

#define OMA_PARAM_MA        (g_stOmaParam.ulMaDays)
#define OMA_PARAM_NO        (g_stOmaParam.fNoThreshold)
#define OMA_PARAM_PO        (g_stOmaParam.fPoThreshold)
#define OMA_PARAM_CD        (g_stOmaParam.ulChkSellDays)
#define OMA_DEFAULT_MA      (24)
#define OMA_DEFAULT_NO      (-0.18F)
#define OMA_DEFAULT_PO      (-0.02F)
#define OMA_DEFAULT_CD      (2)
#define OMA_GET_OFFSET(ulPrice, ulMa)   ((((FLOAT)(ulPrice))/(ulMa))-1)
#define OMA_BASE_MA         (250)

VOID OMA_SetParam(IN ULONG ulCnt, IN FLOAT *afParam)
{
    // set default
    if (0==ulCnt) {
        g_stOmaParam.ulMaDays     = OMA_DEFAULT_MA;
        g_stOmaParam.fNoThreshold = OMA_DEFAULT_NO;
        g_stOmaParam.fPoThreshold = OMA_DEFAULT_PO;
        g_stOmaParam.ulChkSellDays = OMA_DEFAULT_CD;
        return;
    }

    assert(ulCnt == sizeof(OMA_PARAM_S)/sizeof(UINT32));

    g_stOmaParam.ulMaDays     = (ULONG)afParam[0];
    g_stOmaParam.fNoThreshold = afParam[1];
    g_stOmaParam.fPoThreshold = afParam[2];
    g_stOmaParam.ulChkSellDays = (ULONG)afParam[3];

    assert(OMA_PARAM_MA > 1);
    assert(OMA_PARAM_NO <= 0);
    assert(OMA_PARAM_PO > OMA_PARAM_NO);
    assert(OMA_PARAM_CD > 0);

    return;
}

VOID OMA_Statistics(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData)
{
    ULONG ulMa;

    if (ulIndex < OMA_DEFAULT_MA) return;

    ulMa=GetMean(OMA_DEFAULT_MA,pstCurrData,INVAILD_ULONG);

    printf("%lu,%lu,%f\n",pstCurrData->ulDate, pstCurrData->stDailyPrice.ulEnd, 
                        pstCurrData->stDailyPrice.ulEnd/(FLOAT)ulMa-1);

    return;
}
#if 1
BOOL_T OMA_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    ULONG ulMa;
    ULONG ulPrevShorterMa, ulCurrShorterMa;
    FLOAT fCurrOffset;
    FLOAT fThresholdPrice;
    FLOAT fCurrRise, fExpectRise;
    ULONG i;
    ULONG ulStartPrice;
    FILE_WHOLE_DATA_S *pstMeanData;
    
    if (ulIndex < OMA_PARAM_MA) return BOOL_FALSE;

    // without multi factor
    for (i=0,pstMeanData=pstCurrData;i<OMA_PARAM_MA;i++,pstMeanData--) {
        if ((pstMeanData->stFactor.ulFlag == FILE_VAILD_FACTOR) &&
            (pstMeanData->stFactor.fMulti > 1))
           return BOOL_FALSE;
    }
    pstMeanData=pstCurrData-OMA_PARAM_MA+1;
    ulStartPrice = pstMeanData->stDailyPrice.ulEnd;

    ulMa = GetMean(OMA_PARAM_MA, pstCurrData, INVAILD_ULONG);
    fCurrOffset = ((FLOAT)pstCurrData->stDailyPrice.ulEnd/ulMa) - 1;

    if (fCurrOffset >= OMA_PARAM_NO) return BOOL_FALSE;

    //(VOID)GetTotalRise(1,pstCurrData,RISE_TYPE_END,&fCurrRise);
    //if (fCurrRise < (0-STOCK_RISE_THREASHOLD)) return BOOL_FALSE;

    ulPrevShorterMa = GetMean(OMA_PARAM_MA-1,pstCurrData-1,INVAILD_ULONG);
    ulCurrShorterMa = GetMean(OMA_PARAM_MA-1,pstCurrData,ulPrevShorterMa);
    
    //fThresholdPrice = ((OMA_PARAM_NO+1)*ulMa-(OMA_PARAM_NO+1)/OMA_PARAM_MA*ulStartPrice)/
    //                   (1-((OMA_PARAM_NO+1)/OMA_PARAM_MA));
    fThresholdPrice = (FLOAT)ulCurrShorterMa/ulPrevShorterMa*pstCurrData->stDailyPrice.ulEnd;
    fExpectRise = fThresholdPrice/pstCurrData->stDailyPrice.ulEnd-1;
    if (fExpectRise > STOCK_RISE_THREASHOLD) return BOOL_FALSE;
    
    fThresholdPrice = MAX(pstCurrData->stDailyPrice.ulEnd,fThresholdPrice);
    
    // set deal info   
    pstDealInfo->bIsSell    = BOOL_FALSE;
    pstDealInfo->usDealHour = 14;
    pstDealInfo->usDealMin  = (USHORT)RandomUlong(50,60);
    pstDealInfo->bIsHigher  = BOOL_TRUE;    
    pstDealInfo->fWeight    = (FLOAT)RandomUlong(0,100);    // more rise more weight
    //pstDealInfo->fWeight    = fCurrRise;
    pstDealInfo->fThresholdPrice = FILE_PRICE2REAL(fThresholdPrice);

    return BOOL_TRUE;
}
#else

BOOL_T OMA_Choose(IN ULONG ulIndex, IN FILE_WHOLE_DATA_S *pstCurrData, OUT CHOOSE_PRE_DEAL_S *pstDealInfo)
{
    UINT32 i;
    ULONG ulMa, ulPrevMa = INVAILD_ULONG;
    FLOAT fOffset;
    FLOAT fThresholdPrice;
    ULONG ulPrevShorterMa, ulCurrShorterMa;
    BOOL_T bIsNeg = BOOL_FALSE;
    FILE_WHOLE_DATA_S *pstChkData = pstCurrData-OMA_PARAM_MA+1;    
    FILE_WHOLE_DATA_S *pstMeanData;
    
    if (ulIndex < OMA_PARAM_MA*2) return BOOL_FALSE;

    // without multi factor
    for (i=0,pstMeanData=pstCurrData;i<OMA_PARAM_MA;i++,pstMeanData--) {
        if ((pstMeanData->stFactor.ulFlag == FILE_VAILD_FACTOR) &&
            (pstMeanData->stFactor.fMulti > 1))
           return BOOL_FALSE;
    }

    for (i=0;i<OMA_PARAM_MA;i++, pstChkData++) {
        ulMa = GetMean(OMA_PARAM_MA,pstChkData,ulPrevMa);
        fOffset = OMA_GET_OFFSET(pstChkData->stDailyPrice.ulEnd, ulMa);
        if (fOffset < OMA_PARAM_NO) bIsNeg = BOOL_TRUE;
        ulPrevMa = ulMa;
    }

    if (BOOL_FALSE == bIsNeg) return BOOL_FALSE;
    if (fOffset < OMA_PARAM_PO) return BOOL_FALSE;

    ulPrevShorterMa = GetMean(OMA_PARAM_MA-1,pstCurrData-1,INVAILD_ULONG);
    ulCurrShorterMa = GetMean(OMA_PARAM_MA-1,pstCurrData,ulPrevShorterMa);

    fThresholdPrice = (FLOAT)ulCurrShorterMa/ulPrevShorterMa*pstCurrData->stDailyPrice.ulEnd;

    // set deal info   
    pstDealInfo->bIsSell    = BOOL_FALSE;
    pstDealInfo->usDealHour = 14;
    pstDealInfo->usDealMin  = (USHORT)RandomUlong(50,60);
    pstDealInfo->bIsHigher  = BOOL_TRUE;    
    pstDealInfo->fWeight    = (FLOAT)RandomUlong(0,100);    // random weight
    pstDealInfo->fThresholdPrice = FILE_PRICE2REAL(fThresholdPrice);

    return BOOL_TRUE;
}
#endif

ULONG OMA_SortWishList(IN ULONG ulWishCnt, IN SIM_STOCK_DATA_S *pstAllData, INOUT SIM_WISH_LIST_S *astWishList)
{
    return ulWishCnt;
}

ULONG OMA_GetGainPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    return INVAILD_ULONG;
}

#if 0
ULONG OMA_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    ULONG ulHoldDays=0;
    ULONG ulCurrMa, ulPrevMa, ulBuyMa;
    FLOAT fCurrOffset, fPrevOffset, fBuyOffset;
    FILE_WHOLE_DATA_S *pstPrevData = pstCurrData-1;
    FILE_WHOLE_DATA_S *pstBuyData = pstCurrData;

    assert(pstCurrData->ulDate>=pstStockCtrl->ulBuyDate);

    // point to buy date
    while (1) {
        ulHoldDays++;
        if (pstBuyData->ulDate==pstStockCtrl->ulBuyDate) break;
        pstBuyData--;
    }
    ulBuyMa = GetMean(OMA_PARAM_MA, pstBuyData, INVAILD_ULONG);
    fBuyOffset = OMA_GET_OFFSET(pstBuyData->stDailyPrice.ulEnd, ulBuyMa);
    //assert(fBuyOffset > OMA_PARAM_NO);

    ulPrevMa = GetMean(OMA_PARAM_MA, pstPrevData, INVAILD_ULONG);
    ulCurrMa = GetMean(OMA_PARAM_MA, pstCurrData, ulPrevMa);

    fPrevOffset = OMA_GET_OFFSET(pstPrevData->stDailyPrice.ulEnd, ulPrevMa);
    fCurrOffset = OMA_GET_OFFSET(pstCurrData->stDailyPrice.ulEnd, ulCurrMa);

    if ((fCurrOffset < OMA_PARAM_PO) && (fPrevOffset < OMA_PARAM_PO))
        return GetDailyLow(OMA_PARAM_CD, pstBuyData);

    if (fCurrOffset < fPrevOffset)
        return GetDailyLow(OMA_PARAM_CD, pstCurrData);

    return 0;
}
#else
ULONG OMA_GetLossPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    ULONG ulHoldDays=0;
    FILE_WHOLE_DATA_S *pstBuyData = pstCurrData;

    assert(pstCurrData->ulDate>=pstStockCtrl->ulBuyDate);

    // point to buy date
    while (1) {
        ulHoldDays++;
        if (pstBuyData->ulDate==pstStockCtrl->ulBuyDate) break;
        pstBuyData--;
    }

    //if (ulHoldDays < 20) return GetDailyLow(OMA_PARAM_CD, pstBuyData);
    if (ulHoldDays > 30) return (ULONG)((1+(ulHoldDays*0.003F))*pstBuyData->stDailyPrice.ulEnd);
    return (ULONG)(0.8F * pstBuyData->stDailyPrice.ulEnd);
    //return (ULONG)(ulHoldDays*0.002F*pstBuyData->stDailyPrice.ulEnd);
}
#endif
ULONG OMA_GetBuyPrice(IN FILE_WHOLE_DATA_S *pstCurrData, IN ULONG ulWishPrice)
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

ULONG OMA_GetSellPrice(IN FILE_WHOLE_DATA_S *pstCurrData, INOUT STOCK_CTRL_S *pstStockCtrl)
{
    // not sell until reachs threshold
    return INVAILD_ULONG;
}

