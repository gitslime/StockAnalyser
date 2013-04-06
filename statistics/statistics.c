#include "../common/comm.h"
#include "../common/file.h"

#define METHOD_RISE      0x0001

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

#define RISE_CONTINUOUS_DAYS        (3)
#define RISE_WATCH_DAYS             (1)
#define RISE_TYPE_HIGH              (0x02UL)
#define RISE_TYPE_LOW               (0x03UL)
#define RISE_TYPE_END               (0x04UL)

// get total rise of n days before current
BOOL_T STATIC_GetTotalRise(IN ULONG ulCnt, IN FILE_WHOLE_DATA_S *pstCurrent, IN ULONG ulType, OUT FLOAT *pfTotalRise)
{
    ULONG i;
    BOOL_T bIsContinuous = BOOL_TRUE;
    ULONG ulBaseEndPrice;
    ULONG ulHigh, ulLow;
    FLOAT fPrevPrice, fCurrPrice;
    FLOAT fMulti, fAdder;
    FILE_WHOLE_DATA_S *pstBase = pstCurrent - ulCnt - 1;
    FILE_WHOLE_DATA_S *pstTemp = pstCurrent - ulCnt;

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
        else {
            assert(0);
        }
        if (fCurrPrice < fPrevPrice) bIsContinuous = BOOL_FALSE;

        pstTemp++;
        fPrevPrice = fCurrPrice;
    }

    *pfTotalRise = fCurrPrice / ulBaseEndPrice - 1;

    return bIsContinuous;
}

VOID STATIC_Rise(IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstWholeData)
{
    ULONG i;
    BOOL_T bIsCont;
    FLOAT fPrevRise, fWatchRise, fWatchDrop;
    FILE_WHOLE_DATA_S *pstCurr = pstWholeData+RISE_CONTINUOUS_DAYS+1;
    FILE_WHOLE_DATA_S *pstWatch = pstCurr+RISE_WATCH_DAYS;

    ulEntryCnt -= RISE_WATCH_DAYS;
    for (i=RISE_CONTINUOUS_DAYS+1;i<ulEntryCnt;i++, pstCurr++, pstWatch++) {
        bIsCont = STATIC_GetTotalRise(RISE_CONTINUOUS_DAYS, pstCurr, RISE_TYPE_END, &fPrevRise);
        if (BOOL_FALSE == bIsCont) continue;

        STATIC_GetTotalRise(RISE_WATCH_DAYS, pstWatch, RISE_TYPE_HIGH, &fWatchRise);
        STATIC_GetTotalRise(RISE_WATCH_DAYS, pstWatch, RISE_TYPE_LOW, &fWatchDrop);

        printf("%d,%f,%f,%f\n", pstCurr->ulDate, fPrevRise, fWatchRise, fWatchDrop);
    }

    return;
}

VOID STATIC_Distribute(IN ULONG ulCode, IN CHAR *szDir, IN ULONG ulMethod)
{
    ULONG ulEntryCnt;
    FILE_WHOLE_DATA_S *astWholeData = NULL;

    ulEntryCnt = FILE_GetFileData(ulCode, szDir, FILE_TYPE_CUSTOM, &astWholeData);
    if (0 == ulEntryCnt) return;

    switch (ulMethod) {
        case METHOD_RISE:
            STATIC_Rise(ulEntryCnt, astWholeData);
            break;
        default:
            printf("method not support\n");
    }

    free(astWholeData);

    return;
}

int main(int argc,char *argv[]) 
{
    ULONG ulStockCode;
    ULONG ulMethod;
    
    //check parameter
    if ((argc < 4) || (argc > 5)) {
        printf("USAGE: %s method path code [debug]", argv[0]);
        exit(1);
    }

    if (0 == _stricmp(argv[argc-1], "debug"))
        g_bIsDebugMode = BOOL_TRUE;
    
    ulStockCode = (ULONG)atol(argv[3]);
    ulMethod    = GetMethod(argv[1]);

    STATIC_Distribute(ulStockCode, argv[2], ulMethod);
    
    return 0;
}
