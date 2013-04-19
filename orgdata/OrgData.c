#include "../common/comm.h"
#include "../common/file.h"

VOID FILE_UpdateDailyPrice(IN ULONG ulCode, IN const CHAR *szSrcDir, IN const CHAR *szTrgDir)
{
    ULONG i,j;
    ULONG ulSrcEntryCnt, ulOgEntryCnt, ulTrgEntryCnt;
    ULONG ulRemainingCnt;
    FILE_THS_DAILY_ENTRY_S *astDailyData = NULL;
    FILE_THS_DAILY_ENTRY_S *pstDailyData = NULL;
    FILE_WHOLE_DATA_S *astOgData    = NULL;
    FILE_WHOLE_DATA_S *pstOgData    = NULL;
    FILE_WHOLE_DATA_S *astWholeData = NULL;
    FILE_WHOLE_DATA_S *pstWholeData = NULL;

    assert(NULL != szSrcDir);
    assert(NULL != szTrgDir);

    //get source data and original data
    ulSrcEntryCnt = FILE_GetFileData(ulCode, szSrcDir, FILE_TYPE_THS_DAY, &astDailyData);
    if (0==ulSrcEntryCnt) return;
    
    //original file maybe not exist
    ulOgEntryCnt = FILE_GetFileData(ulCode, szTrgDir, FILE_TYPE_CUSTOM, &astOgData);

    astWholeData = malloc(sizeof(FILE_WHOLE_DATA_S) * (ulSrcEntryCnt+ulOgEntryCnt));
    assert(NULL != astWholeData);
    memset(astWholeData, 0xFF, sizeof(FILE_WHOLE_DATA_S) * (ulSrcEntryCnt+ulOgEntryCnt));

    if (0 == ulOgEntryCnt) {
        astOgData = malloc(sizeof(FILE_WHOLE_DATA_S));
        assert(NULL != astOgData);
        memset(astOgData, 0xFF, sizeof(FILE_WHOLE_DATA_S));
    }

    pstDailyData = astDailyData;
    pstOgData    = astOgData;
    pstWholeData = astWholeData;

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
        
        //Set data no matter it is already had
        FILE_ThsDayToCustom(pstDailyData, pstWholeData);
        pstWholeData++;
    }

    // add remaining original data
    ulRemainingCnt = ulOgEntryCnt-(pstOgData-astOgData);
    if (0 != ulRemainingCnt) {
        memcpy(pstWholeData, pstOgData, ulRemainingCnt*sizeof(FILE_WHOLE_DATA_S));
        pstWholeData += ulRemainingCnt;
    }

    // write file
    ulTrgEntryCnt = pstWholeData - astWholeData;
    FILE_SetFileData(ulCode, szTrgDir, FILE_TYPE_CUSTOM, sizeof(FILE_WHOLE_DATA_S)*ulTrgEntryCnt,astWholeData);
    
    free(astDailyData);
    free(astOgData);
    free(astWholeData);
    
    return;
}

VOID FILE_UpdateMin30Price(IN ULONG ulCode, IN const CHAR *szSrcDir, IN const CHAR *szTrgDir)
{
    ULONG i, j;
    ULONG ulSrcEntryCnt, ulOgEntryCnt;
    ULONG ulCurrDate;
    FILE_THS_MIN5_ENTRY_S  *astMin5Data  = NULL;
    FILE_THS_MIN5_ENTRY_S  *pstMin5Data  = NULL;
    FILE_WHOLE_DATA_S *astOgData    = NULL;
    FILE_WHOLE_DATA_S *pstOgData    = NULL;

    assert(NULL != szSrcDir);
    assert(NULL != szTrgDir);

    ulSrcEntryCnt = FILE_GetFileData(ulCode, szSrcDir, FILE_TYPE_THS_MIN5, &astMin5Data);
    if (0 == ulSrcEntryCnt) return;
    
    ulOgEntryCnt  = FILE_GetFileData(ulCode, szTrgDir, FILE_TYPE_CUSTOM, &astOgData);
    assert(0!=ulOgEntryCnt);

    i = j = 0;
    pstMin5Data = astMin5Data;
    pstOgData   = astOgData;

    // set min5 price to Original data
    while (i<ulSrcEntryCnt) {
        ulCurrDate = FILE_ThsMin2Date(pstMin5Data->ulMin);

        // get current date's entry in original data
        // price maybe not continuous
        for (;j<ulOgEntryCnt;j++,pstOgData++) {
            if (ulCurrDate == pstOgData->ulDate) break;
        }

        if (ulCurrDate != pstOgData->ulDate) {
            //min 5 date is not in the original date
            printf("daily entry of %d is not exist\n", ulCurrDate);
            break;
        }

        i += FILE_ThsMin5ToMin30(pstMin5Data, pstOgData);
        pstMin5Data=astMin5Data+i;
    }
    
    // write file
    FILE_SetFileData(ulCode, szTrgDir, FILE_TYPE_CUSTOM, sizeof(FILE_WHOLE_DATA_S)*ulOgEntryCnt, astOgData);
    
    free(astMin5Data);
    free(astOgData);
    
    return;
}

VOID FILE_UpdateWeight(IN ULONG ulCode, IN const CHAR *szSrcDir, IN const CHAR *szTrgDir)
{
    ULONG i;
    ULONG ulDate, ulOffset, ulUpdateCnt;
    ULONG ulSrcEntryCnt, ulOgEntryCnt;
    FILE_QL_WEIGHT_ENTRY_S *astWgtData = NULL;
    FILE_WHOLE_DATA_S  *astOgData  = NULL;
    FILE_QL_WEIGHT_ENTRY_S *pstWgtData = NULL;
    FILE_WHOLE_DATA_S  *pstOgData  = NULL;
    FILE *fpSrcFile = NULL;
    FILE *fpTrgFile = NULL;

    assert(NULL != szSrcDir);
    assert(NULL != szTrgDir);

    ulSrcEntryCnt = FILE_GetFileData(ulCode, szSrcDir, FILE_TYPE_QL_WGT, &astWgtData);
    if (0 == ulSrcEntryCnt) return;

    ulOgEntryCnt  = FILE_GetFileData(ulCode, szTrgDir, FILE_TYPE_CUSTOM, &astOgData);
    assert(0!=ulOgEntryCnt);
    
    pstWgtData   = astWgtData;
    pstOgData    = astOgData;
    for (i=0,ulUpdateCnt=0;i<ulSrcEntryCnt;i++,pstWgtData++) {
        ulDate = FILE_QlDate2Custom(pstWgtData->ulQlDate);
        ulOffset = GetIndexByDate(ulDate,INDEX_EXACT,ulOgEntryCnt,astOgData);
        if (INVAILD_ULONG == ulOffset) continue;

        pstOgData = astOgData + ulOffset;
        ulUpdateCnt += FILE_QlWeightToCustom(pstWgtData, pstOgData);
    }

    if (0 != ulUpdateCnt) {
        // write file
        FILE_SetFileData(ulCode, szTrgDir, FILE_TYPE_CUSTOM, 
                         sizeof(FILE_WHOLE_DATA_S)*ulOgEntryCnt, astOgData);
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


