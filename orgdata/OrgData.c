#include "comm.h"












ULONG FILE_WeightDate2Price(IN ULONG ulWeightDate)
{
#define FILE_WEIGHT_GET_YEAR(ulDate)    (((ulDate) & 0xFFF00000) >> 20)
#define FILE_WEIGHT_GET_MONTH(ulDate)   (((ulDate) & 0x000F0000) >> 16)
#define FILE_WEIGHT_GET_DAY(ulDate)     (((ulDate) & 0x0000F800) >> 11)

    ULONG ulYear  = FILE_WEIGHT_GET_YEAR(ulWeightDate);
    ULONG ulMonth = FILE_WEIGHT_GET_MONTH(ulWeightDate);
    ULONG ulDay   = FILE_WEIGHT_GET_DAY(ulWeightDate);

    return ((ulYear * 10000) + (ulMonth * 100) + ulDay);
}

ULONG FILE_Weight2Factor(IN FILE_WEIGHT_DATA_S *pstWeight, OUT FACTOR_S *pstFactor)
{
    BOOL_T bVaild;
    FLOAT fMultiTemp, fAdderTemp;

    bVaild = BOOL_FALSE;
    fMultiTemp = 1;
    fAdderTemp = 0;

    if (0 != pstWeight->ulProfit) {
        fAdderTemp = (FLOAT)(pstWeight->ulProfit / 10);
        bVaild = BOOL_TRUE;
    }

    if ((0 != pstWeight->ulAdd) || (0 != pstWeight->ulGift) || (0 != pstWeight->ulOffer)) {
        fMultiTemp  = ((FLOAT)(pstWeight->ulAdd + pstWeight->ulGift + pstWeight->ulOffer + 100000) / 100000);
        fAdderTemp -= (FLOAT)(pstWeight->ulOffer * pstWeight->ulOfferPrice / 100000);

        bVaild = BOOL_TRUE;
    }

    if (BOOL_FALSE == bVaild) {
        return 0;
    }

    // Check original data
    if (FILE_VAILD_FACTOR == pstFactor->ulFlag) {
        assert(pstFactor->fMulti == fMultiTemp);
        assert(pstFactor->fAdder == fAdderTemp);

        return 0;
    }
    
    pstFactor->ulFlag = FILE_VAILD_FACTOR;
    pstFactor->fMulti = fMultiTemp;
    pstFactor->fAdder = fAdderTemp;

    return 1;
}


VOID FILE_EntryCheck(IN ULONG ulEntryCnt, IN FILE_WHOLE_DATA_S *pstWholeEntry)
{
    ULONG i;
    FILE_WHOLE_DATA_S *pstNextEntry = pstWholeEntry+1;

    for (i=1; i< ulEntryCnt; i++,pstWholeEntry++,pstNextEntry++) {
        assert(pstNextEntry->ulDate > pstWholeEntry->ulDate);
    }
    return;
}

VOID FILE_UpdateDailyPrice(IN ULONG ulCode, IN const CHAR *szSrcDir, IN const CHAR *szTrgDir)
{
    ULONG i,j;
    ULONG ulSrcEntryCnt, ulOgEntryCnt, ulTrgEntryCnt;
    ULONG ulRemainingCnt;
    FILE_DAILY_DATA_S *astDailyData = NULL;
    FILE_WHOLE_DATA_S *astOgData    = NULL;
    FILE_WHOLE_DATA_S *astWholeData = NULL;
    FILE_DAILY_DATA_S *pstDailyData = NULL;
    FILE_WHOLE_DATA_S *pstOgData    = NULL;
    FILE_WHOLE_DATA_S *pstWholeData = NULL;
    FILE *fpSrcFile = NULL;
    FILE *fpTrgFile = NULL;

    assert(NULL != szSrcDir);
    assert(NULL != szTrgDir);

    fpSrcFile = FILE_OpenDataWithoutHead(ulCode, szSrcDir, FILE_FLAG_DAY);
    fpTrgFile = FILE_OpenDataWithoutHead(ulCode, szTrgDir, FILE_FLAG_ALL_R);
    if ((NULL == fpSrcFile) || (NULL == fpTrgFile))
        return;

    ulSrcEntryCnt = FILE_GetEntryCnt(fpSrcFile, FILE_FLAG_DAY);
    ulOgEntryCnt  = FILE_GetEntryCnt(fpTrgFile, FILE_FLAG_ALL_R);
    assert(0!=ulSrcEntryCnt);

    astDailyData = malloc(sizeof(FILE_DAILY_DATA_S) * ulSrcEntryCnt);
    astOgData    = malloc(sizeof(FILE_WHOLE_DATA_S) * MAX(ulOgEntryCnt, 1));    // at least one data
    astWholeData = malloc(sizeof(FILE_WHOLE_DATA_S) * (ulSrcEntryCnt+ulOgEntryCnt));
    assert(NULL != astDailyData);
    assert(NULL != astOgData);
    assert(NULL != astWholeData);
    pstDailyData = astDailyData;
    pstOgData    = astOgData;
    pstWholeData = astWholeData;
    memset(astWholeData, 0xFF, sizeof(FILE_WHOLE_DATA_S) * (ulSrcEntryCnt+ulOgEntryCnt));

    fread(astDailyData, sizeof(FILE_DAILY_DATA_S), ulSrcEntryCnt, fpSrcFile);
    fclose(fpSrcFile);

    if (0 != ulOgEntryCnt) {
        fread(astOgData, sizeof(FILE_WHOLE_DATA_S), ulOgEntryCnt, fpTrgFile);
    }
    else {
        memset(astOgData, 0xFF, sizeof(FILE_WHOLE_DATA_S));
    }
    fclose(fpTrgFile);


    // set daily price to target data
    for (i=0,j=0;i<ulSrcEntryCnt;i++,pstDailyData++) {
        if ((0 == pstDailyData->ulDate) || 
            (0x80000000 == pstDailyData->ulVolWithCheck)) continue;  //invaild data

        // if target data is before source data, copy this part first
        // src data maybe not continous
        for (;j<ulOgEntryCnt;j++) {
            if (pstDailyData->ulDate > pstOgData->ulDate) {
                memcpy(pstWholeData, pstOgData, sizeof(FILE_WHOLE_DATA_S));
                pstWholeData++;
                pstOgData++;
            }
            else {
                break;
            }
        }

        if (pstDailyData->ulDate == pstOgData->ulDate) {
            memcpy(pstWholeData, pstOgData, sizeof(FILE_WHOLE_DATA_S));
            pstOgData++;
            j++;
        }
        
        //Set data no matter it is alread had
        sprintf_s(pstWholeData->szComment, 16, "%d", pstDailyData->ulDate);
        pstWholeData->ulDate = pstDailyData->ulDate;
        pstWholeData->ulRsv  = INVAILD_ULONG;
        pstWholeData->stDailyPrice.ulBegin = pstDailyData->ulBeginWithCheck - FILE_DATA_PRICE_CHK;
        pstWholeData->stDailyPrice.ulHigh  = pstDailyData->ulHighWithCheck  - FILE_DATA_PRICE_CHK;
        pstWholeData->stDailyPrice.ulLow   = pstDailyData->ulLowWithCheck   - FILE_DATA_PRICE_CHK;
        pstWholeData->stDailyPrice.ulEnd   = pstDailyData->ulEndWithCheck   - FILE_DATA_PRICE_CHK;
        pstWholeData->stDailyPrice.ulVol   = FILE_GetVol(pstDailyData->ulVolWithCheck);
        pstWholeData->stDailyPrice.ulFlag  = FILE_VAILD_PRICE;
        pstWholeData++;
    }

    // add remaining original data
    ulRemainingCnt = ulOgEntryCnt-(pstOgData-astOgData);
    if (0 != ulRemainingCnt) {
        memcpy(pstWholeData, pstOgData, ulRemainingCnt*sizeof(FILE_WHOLE_DATA_S));
        pstWholeData += ulRemainingCnt;
    }

    // check whole data
    ulTrgEntryCnt = pstWholeData - astWholeData;
    FILE_EntryCheck(ulTrgEntryCnt, astWholeData);

    // reopen for write
    fpTrgFile = FILE_OpenDataWithoutHead(ulCode, szTrgDir, FILE_FLAG_ALL_W);
    fwrite(astWholeData, sizeof(FILE_WHOLE_DATA_S), ulTrgEntryCnt, fpTrgFile);
    fclose(fpTrgFile);
    
    free(astDailyData);
    free(astOgData);
    free(astWholeData);
    
    return;
}

VOID FILE_UpdateMin30Price(IN ULONG ulCode, IN const CHAR *szSrcDir, IN const CHAR *szTrgDir)
{
    ULONG i, k;
    ULONG ulSrcEntryCnt, ulOgEntryCnt, ulTrgEntryCnt;
    ULONG ulRemainingCnt;
    ULONG ulPrevDate, ulCurrDate;
    ULONG ulMaxPrice, ulMinPrice, ulMin30Vol;
    FILE_MIN5_DATA_S  *astMin5Data  = NULL;
    FILE_WHOLE_DATA_S *astOgData    = NULL;
    FILE_WHOLE_DATA_S *astWholeData = NULL;
    FILE_MIN5_DATA_S  *pstMin5Data  = NULL;
    FILE_WHOLE_DATA_S *pstOgData    = NULL;
    FILE_WHOLE_DATA_S *pstWholeData = NULL;
    PRICE_S *pstMin30Price = NULL;
    FILE *fpSrcFile = NULL;
    FILE *fpTrgFile = NULL;

    assert(NULL != szSrcDir);
    assert(NULL != szTrgDir);

    fpSrcFile = FILE_OpenDataWithoutHead(ulCode, szSrcDir, FILE_FLAG_MIN5);
    fpTrgFile = FILE_OpenDataWithoutHead(ulCode, szTrgDir, FILE_FLAG_ALL_R);
    if ((NULL == fpSrcFile) || (NULL == fpTrgFile))
        return;

    ulSrcEntryCnt = FILE_GetEntryCnt(fpSrcFile, FILE_FLAG_MIN5);
    ulOgEntryCnt  = FILE_GetEntryCnt(fpTrgFile, FILE_FLAG_ALL_R);
    assert(0!=ulSrcEntryCnt);

    astMin5Data  = malloc(sizeof(FILE_MIN5_DATA_S) * ulSrcEntryCnt);
    astOgData    = malloc(sizeof(FILE_WHOLE_DATA_S) * MAX(ulOgEntryCnt, 1));    // at least one data
    astWholeData = malloc(sizeof(FILE_WHOLE_DATA_S) * (ulSrcEntryCnt+ulOgEntryCnt));
    assert(NULL != astMin5Data);
    assert(NULL != astOgData);
    assert(NULL != astWholeData);
    pstMin5Data  = astMin5Data;
    pstOgData    = astOgData;
    pstWholeData = astWholeData;
    memset(astWholeData, 0xFF, sizeof(FILE_WHOLE_DATA_S) * (ulSrcEntryCnt+ulOgEntryCnt));

    fread(astMin5Data, sizeof(FILE_MIN5_DATA_S), ulSrcEntryCnt, fpSrcFile);
    fclose(fpSrcFile);

    if (0 != ulOgEntryCnt) {
        fread(astOgData, sizeof(FILE_WHOLE_DATA_S), ulOgEntryCnt, fpTrgFile);
    }
    else {
        memset(astOgData, 0xFF, sizeof(FILE_WHOLE_DATA_S));
    }
    fclose(fpTrgFile);

    // set min5 price to target data
    for (i=0,k=0,ulPrevDate=0,
         ulMaxPrice=0,ulMinPrice=0xFFFFFFFF,ulMin30Vol=0;
         i<ulSrcEntryCnt;i++,pstMin5Data++) {
        ulCurrDate = FILE_Min2Date(pstMin5Data->ulMin);

        // copy original data before current day
        // prevent min5 price is not continuous
        while (((ULONG)(pstOgData-astOgData)<ulOgEntryCnt) && (pstOgData->ulDate < ulCurrDate)) {
            memcpy(pstWholeData, pstOgData, sizeof(FILE_WHOLE_DATA_S));
            pstOgData++;
            pstWholeData++;
        }

        // new day begin
        if (ulPrevDate != ulCurrDate) {
            // make sure daily data already had
            if (ulCurrDate != pstOgData->ulDate) {
                printf("daily data of %d is not exist\n", ulCurrDate);
                break;
            }

            memcpy(pstWholeData, pstOgData, sizeof(FILE_WHOLE_DATA_S));
            pstMin30Price = pstWholeData->astMin30Price;

            // shift to next day
            ulPrevDate = ulCurrDate;
            pstOgData++;
            pstWholeData++;
        }
      
        k++;
    
        /* get min&max price in the section */
        if (ulMaxPrice < pstMin5Data->ulHighWithCheck) {
            ulMaxPrice = pstMin5Data->ulHighWithCheck;
        }
        if (ulMinPrice > pstMin5Data->ulLowWithCheck) {
            ulMinPrice = pstMin5Data->ulLowWithCheck;
        }

        ulMin30Vol += FILE_GetVol(pstMin5Data->ulVolWithCheck);

        if (1 == k) {
            pstMin30Price->ulBegin = pstMin5Data->ulBeginWithCheck - FILE_DATA_PRICE_CHK;
        }

        /* 30min = 6 * 5min */
        if (6 == k) {
            pstMin30Price->ulHigh = ulMaxPrice - FILE_DATA_PRICE_CHK;
            pstMin30Price->ulLow  = ulMinPrice - FILE_DATA_PRICE_CHK;
            pstMin30Price->ulEnd  = pstMin5Data->ulEndWithCheck - FILE_DATA_PRICE_CHK;
            pstMin30Price->ulVol  = ulMin30Vol+3; /* each data have 0.5 offset */
            pstMin30Price->ulFlag = FILE_VAILD_PRICE;

            pstMin30Price++;
            ulMin30Vol = 0;
            k = 0;
            ulMaxPrice = 0;
            ulMinPrice = 0xFFFFFFFF;
        }
    }

    // add remaining original data
    ulRemainingCnt = ulOgEntryCnt-(pstOgData-astOgData);
    if (0 != ulRemainingCnt) {
        memcpy(pstWholeData, pstOgData, ulRemainingCnt*sizeof(FILE_WHOLE_DATA_S));
        pstWholeData += ulRemainingCnt;
    }

    // reopen for write
    ulTrgEntryCnt = pstWholeData - astWholeData;
    fpTrgFile = FILE_OpenDataWithoutHead(ulCode, szTrgDir, FILE_FLAG_ALL_W);
    fwrite(astWholeData, sizeof(FILE_WHOLE_DATA_S), ulTrgEntryCnt, fpTrgFile);
    fclose(fpTrgFile);
    
    free(astMin5Data);
    free(astOgData);
    free(astWholeData);
    
    return;
}

VOID FILE_UpdateWeight(IN ULONG ulCode, IN const CHAR *szSrcDir, IN const CHAR *szTrgDir)
{
    ULONG i;
    ULONG ulPriceDate, ulOffset, ulUpdateCnt;
    ULONG ulSrcEntryCnt, ulOgEntryCnt;
    FILE_WEIGHT_DATA_S *astWgtData = NULL;
    FILE_WHOLE_DATA_S  *astOgData  = NULL;
    FILE_WEIGHT_DATA_S *pstWgtData = NULL;
    FILE_WHOLE_DATA_S  *pstOgData  = NULL;
    FILE *fpSrcFile = NULL;
    FILE *fpTrgFile = NULL;

    assert(NULL != szSrcDir);
    assert(NULL != szTrgDir);

    fpSrcFile = FILE_OpenDataWithoutHead(ulCode, szSrcDir, FILE_FLAG_WGT);
    fpTrgFile = FILE_OpenDataWithoutHead(ulCode, szTrgDir, FILE_FLAG_ALL_R);
    if ((NULL == fpSrcFile) || (NULL == fpTrgFile))
        return;

    ulSrcEntryCnt = FILE_GetEntryCnt(fpSrcFile, FILE_FLAG_WGT);
    ulOgEntryCnt  = FILE_GetEntryCnt(fpTrgFile, FILE_FLAG_ALL_R);
    assert(0!=ulSrcEntryCnt);
    assert(0!=ulOgEntryCnt);        //no sense to get weight without price

    astWgtData   = malloc(sizeof(FILE_WEIGHT_DATA_S) * ulSrcEntryCnt);
    astOgData    = malloc(sizeof(FILE_WHOLE_DATA_S)  * ulOgEntryCnt);
    assert(NULL != astWgtData);
    assert(NULL != astOgData);
    pstWgtData   = astWgtData;
    pstOgData    = astOgData;

    fread(astWgtData, sizeof(FILE_WEIGHT_DATA_S), ulSrcEntryCnt, fpSrcFile);
    fclose(fpSrcFile);

    fread(astOgData, sizeof(FILE_WHOLE_DATA_S), ulOgEntryCnt, fpTrgFile);
    fclose(fpTrgFile);

    for (i=0,ulUpdateCnt=0;i<ulSrcEntryCnt;i++,pstWgtData++) {
        ulPriceDate = FILE_WeightDate2Price(pstWgtData->ulDate);
        ulOffset = GetIndexByDate(ulPriceDate,ulOgEntryCnt,astOgData);
        if (INVAILD_ULONG == ulOffset) continue;

        pstOgData = astOgData + ulOffset;
        ulUpdateCnt += FILE_Weight2Factor(pstWgtData, &(pstOgData->stFactor));
    }

    if (0 != ulUpdateCnt) {
        // reopen for write
        fpTrgFile = FILE_OpenDataWithoutHead(ulCode, szTrgDir, FILE_FLAG_ALL_W);
        fwrite(astOgData, sizeof(FILE_WHOLE_DATA_S), ulOgEntryCnt, fpTrgFile);
        fclose(fpTrgFile);
    }
    
    free(astWgtData);
    free(astOgData);

    return;
}


int main(int argc,char *argv[]) 
{
    ULONG ulStockCode;
    
    //check parameter
    if ((argc < 4) || (argc > 6)) {
        printf("USAGE: %s code source_path target_path [debug] [verbose]", argv[0]);
        exit(1);
    }
    ulStockCode = (ULONG)atol(argv[1]);

    if ((0 == _stricmp(argv[argc-1], "debug")) || (0 == _stricmp(argv[argc-2], "debug")))
        g_bIsDebugMode = BOOL_TRUE;

    //orgnize Data
    FILE_UpdateDailyPrice(ulStockCode, argv[2], argv[3]);
    FILE_UpdateMin30Price(ulStockCode, argv[2], argv[3]);
    FILE_UpdateWeight(ulStockCode, argv[2], argv[3]);

    return 0;
}


