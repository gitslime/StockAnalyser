#include "../common/comm.h"
#include "../common/file.h"

/* range is defined by bit shift
   for example, range=2 means -1/2~+1/2 range=4 means -1/16~+1/16 */
BOOL_T CHECK_IsPriceFit(IN ULONG ulCheckPrice, IN ULONG ulBasePrice, IN ULONG ulRange)
{
    ULONG ulPriceHigh = ulBasePrice + (ulBasePrice>>ulRange);   // '>>' is faster than '/'
    ULONG ulPriceLow  = ulBasePrice - (ulBasePrice>>ulRange);
    return ((ulCheckPrice < ulPriceLow) || (ulCheckPrice > ulPriceHigh)) ? BOOL_FALSE : BOOL_TRUE;
}

VOID CHECK_CheckOne(IN ULONG ulCode, IN ULONG ulStartDate, IN CHAR *szDir)
{
    ULONG i, j, ulEntryCnt;
    ULONG ulHigh, ulLow, ulBegin, ulEnd, ulVol;
    FILE_WHOLE_DATA_S *astWholeData = NULL;
    FILE_WHOLE_DATA_S *pstCurrData = NULL;
    FILE_WHOLE_DATA_S *pstPrevData = NULL;
    PRICE_S *pstMin30Price = NULL;
    PRICE_S *pstDailyPrice = NULL;

    ulEntryCnt = FILE_GetFileData(ulCode, szDir, FILE_TYPE_CUSTOM, &astWholeData);
    if (0 == ulEntryCnt) {
        printf("can't get data\n");
        return;
    }

    // check start date
    if (ulStartDate != astWholeData[0].ulDate) {
        printf("start date = %u, data has %u\n", ulStartDate, astWholeData[0].ulDate);
    }

    // check daily price
    pstPrevData = astWholeData;
    pstCurrData = pstPrevData+1;
    for (i=1;i<ulEntryCnt;i++,pstPrevData++,pstCurrData++) {
        if (INVAILD_ULONG != pstCurrData->ulRsv) {
            printf("%u data invalid!\n", pstCurrData->ulDate);
        }

        if (pstCurrData->ulDate <= pstPrevData->ulDate) {
            printf("%u data reverse!\n",  pstCurrData->ulDate);
        }
    }

    // check min30 price
    pstCurrData = astWholeData;
    for (i=0;i<ulEntryCnt;i++,pstCurrData++) {
        if (FILE_VAILD_PRICE == pstCurrData->astMin30Price[0].ulFlag) {
            DebugOutString("MIN30 starts at %u\n", pstCurrData->ulDate);
            break;
        }
    }

    for (;i<ulEntryCnt;i++,pstCurrData++) {
        pstMin30Price = pstCurrData->astMin30Price;
        ulBegin = pstMin30Price->ulBegin;
        ulHigh  = pstMin30Price->ulHigh;
        ulLow   = pstMin30Price->ulLow;
        ulVol = 0;
        for (j=0;j<8;j++, pstMin30Price++) {
            if (FILE_VAILD_PRICE != pstMin30Price->ulFlag) break;

            ulHigh = MAX(ulHigh, pstMin30Price->ulHigh);
            ulLow  = MIN(ulLow,  pstMin30Price->ulLow);
            ulEnd  = pstMin30Price->ulEnd;
            ulVol += pstMin30Price->ulVol;
        }

        if (0 == j) {
            printf("%u min30 price miss!\n", pstCurrData->ulDate);
            continue;
        }

        // each price has different range
        pstDailyPrice = &(pstCurrData->stDailyPrice);
        if (BOOL_FALSE == CHECK_IsPriceFit(ulBegin, pstDailyPrice->ulBegin, 3))
            printf("%u begin price not march! %u VS %u\n", pstCurrData->ulDate, ulBegin, pstDailyPrice->ulBegin);
        if (BOOL_FALSE == CHECK_IsPriceFit(ulHigh, pstDailyPrice->ulHigh, 6))
            printf("%u high price not march! %u VS %u\n", pstCurrData->ulDate, ulHigh, pstDailyPrice->ulHigh);
        if (BOOL_FALSE == CHECK_IsPriceFit(ulLow, pstDailyPrice->ulLow, 6))
            printf("%u low price not march! %u VS %u\n", pstCurrData->ulDate, ulLow, pstDailyPrice->ulLow);
        if (BOOL_FALSE == CHECK_IsPriceFit(ulEnd, pstDailyPrice->ulEnd, 4))
            printf("%u end price not march! %u VS %u\n", pstCurrData->ulDate, ulEnd, pstDailyPrice->ulEnd);
        if ((pstDailyPrice->ulVol < ulVol-25) || (pstDailyPrice->ulVol > ulVol+10))
            printf("%u volume not march! %u VS %u\n", pstCurrData->ulDate, pstDailyPrice->ulVol, ulVol);
    }
    free(astWholeData);
    
    return;
}

int main(int argc,char *argv[]) 
{
    ULONG i, ulCodeCnt;
    ULONG ulStartDate;
    ULONG ulWgtCnt;
    ULONG *pulCodeList = NULL;
    FILE_QL_WEIGHT_ENTRY_S *astWgtData = NULL;

    //check parameter
    if ((argc < 4) || (argc > 5)) {
        printf("USAGE: %s data-path weight-path { code | all } [debug]", argv[0]);
        exit(1);
    }

    if (0 == _stricmp(argv[argc-1], "debug"))
        g_bIsDebugMode = BOOL_TRUE;

    ulCodeCnt = GetCodeList(argv[3], &pulCodeList);

    for (i=0;i<ulCodeCnt;i++) {
        ulWgtCnt = FILE_GetFileData(pulCodeList[i], argv[2], FILE_TYPE_QL_WGT, &astWgtData);
        if (0 == ulWgtCnt) {
            printf("weight of %06u not exist!\n", pulCodeList[i]);
            continue;
        }

        ulStartDate = FILE_QlDate2Custom(astWgtData[0].ulQlDate);
        free(astWgtData);

        DebugOutString("========%06u========\n", pulCodeList[i]);
        CHECK_CheckOne(pulCodeList[i], ulStartDate, argv[1]);
    }

    return 0;
}

