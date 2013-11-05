#include "comm.h"
#include "file.h"

#define FILE_NAME_LEN       256
#define FILE_GET_DIR_BY_CODE(ulStockCode)   ((ulStockCode) < 600000) ? "sznse" : "shase"
#define FILE_THS_DAY_HEAD_SIZE          (0xB8)
#define FILE_THS_MIN5_HEAD_SIZE         (0x98)

VOID FILE_GetFileName(IN ULONG ulStockCode, IN const CHAR* szFileDir, IN ULONG ulFlag, OUT CHAR* szFileName)
{
    CHAR *pcDir = FILE_GET_DIR_BY_CODE(ulStockCode);

    assert(NULL != szFileDir);
    assert(NULL != szFileName);
    
    szFileName[0] = 0;
    switch (ulFlag) {
        case FILE_TYPE_THS_MIN5:
            sprintf_s(szFileName, FILE_NAME_LEN, "%s/%s/%s/%06lu.%s", szFileDir, pcDir, "min5", ulStockCode, "mn5"); 
            break;
        case FILE_TYPE_THS_DAY:
            sprintf_s(szFileName, FILE_NAME_LEN, "%s/%s/%s/%06lu.%s", szFileDir, pcDir, "day", ulStockCode, "day"); 
            break;
        case FILE_TYPE_QL_WGT:
            sprintf_s(szFileName, FILE_NAME_LEN, "%s/%s/weight/%06lu.wgt", szFileDir, pcDir, ulStockCode);
            break;
        case FILE_TYPE_CUSTOM:
            sprintf_s(szFileName, FILE_NAME_LEN, "%s/%06lu.all", szFileDir, ulStockCode); 
            break;
        default:
            assert(0);
            return;
    }

    return;
}

ULONG FILE_GetFileSize(IN FILE *fp)
{
    LONG lCurrPos = ftell(fp);
    LONG lTotalLen;

    assert(NULL != fp);

    //put to file end
    fseek(fp, 0L, SEEK_END);

    lTotalLen = ftell(fp);

    /* get back file pointer */
    fseek(fp, lCurrPos, SEEK_SET);

    return (ULONG)lTotalLen;
}

ULONG FILE_GetEntrySize(IN ULONG ulFileType)
{
    ULONG ulEntrySize;
    
    switch (ulFileType) {
        case FILE_TYPE_THS_MIN5:
            ulEntrySize = sizeof(FILE_THS_MIN5_ENTRY_S);
            break;
        case FILE_TYPE_THS_DAY:
            ulEntrySize = sizeof(FILE_THS_DAILY_ENTRY_S);
            break;
        case FILE_TYPE_QL_WGT:
            ulEntrySize = sizeof(FILE_QL_WEIGHT_ENTRY_S);
            break;
        case FILE_TYPE_CUSTOM:
            ulEntrySize = sizeof(FILE_WHOLE_DATA_S);
            break;
        default:
            assert(0);
            return 0;
    }

    return ulEntrySize;
}

ULONG FILE_GetFileHead(IN ULONG ulFlag)
{
    ULONG ulFileHead;

    switch (ulFlag) {
        case FILE_TYPE_THS_MIN5:
            ulFileHead  = FILE_THS_MIN5_HEAD_SIZE;
            break;
        case FILE_TYPE_THS_DAY:
            ulFileHead  = FILE_THS_DAY_HEAD_SIZE;
            break;
        case FILE_TYPE_QL_WGT:
        case FILE_TYPE_CUSTOM:
            ulFileHead  = 0;
            break;
        default:
            assert(0);
            return 0;
    }
    
    return ulFileHead;
}

//user need free *ppFileData if return value is not zero
ULONG FILE_GetFileData(IN ULONG ulStockCode, IN const CHAR *szDir, IN ULONG ulFileType, OUT VOID **ppFileData)
{
    ULONG ulFileHead;
    ULONG ulEntrySize;
    ULONG ulFileSize;
    ULONG ulEntryCnt;
    CHAR  szFileName[FILE_NAME_LEN];
    FILE *fp = NULL;

    assert(NULL != ppFileData);

    FILE_GetFileName(ulStockCode, szDir, ulFileType, szFileName);
    fopen_s(&fp, szFileName, "rb");
    if (NULL == fp) {
        printf("invalid file %s\n", szFileName);
        return 0;
    }

    ulFileSize  = FILE_GetFileSize(fp);
    ulFileHead  = FILE_GetFileHead(ulFileType);
    ulEntrySize = FILE_GetEntrySize(ulFileType);
    DebugOutString("code=%lu, file size=%lu, file head=%lu, entry size=%lu\n",
                   ulStockCode,ulFileSize,ulFileHead,ulEntrySize);
    
    ulFileSize -= ulFileHead;
    assert(0 == (ulFileSize%ulEntrySize));
    ulEntryCnt = ulFileSize/ulEntrySize;
    assert(0!=ulEntryCnt);
    DebugOutString("file %s entry=%lu\n", szFileName, ulEntryCnt);

    *ppFileData = malloc(ulFileSize);
    assert(NULL != *ppFileData);
    
    /* Offset the head to read pure data */
    fseek(fp, ulFileHead, SEEK_SET);
    fread(*ppFileData, ulFileSize, 1, fp);

    fclose(fp);

    return ulEntryCnt;
}

VOID FILE_SetFileData
(
    IN ULONG ulStockCode, 
    IN const CHAR *szDir, 
    IN ULONG ulFileType,
    IN ULONG ulSetLen,
    IN VOID  *pFileData
)
{
    CHAR  szFileName[FILE_NAME_LEN];
    FILE *fp = NULL;

    assert(NULL != pFileData);
    assert(FILE_TYPE_CUSTOM == ulFileType);     //Only support custom files now

    FILE_GetFileName(ulStockCode, szDir, ulFileType, szFileName);
    fopen_s(&fp, szFileName, "wb");
    if (NULL == fp) {
        printf("invalid file %s\n", szFileName);
        return;
    }

    fwrite(pFileData, ulSetLen, 1, fp);
    fclose(fp);
    
    return;
}

ULONG FILE_GetVol(IN ULONG ulFileVol)
{
    ULONG ulMulti = ulFileVol & 0xF0000000;
    ULONG ulValue = ulFileVol & 0x0FFFFFFF;

    switch (ulMulti)
    {
        case 0x10000000:
            ulMulti = 10;
            break;
        case 0x00000000:
            ulMulti = 100;
            break;
        case 0x90000000:
            ulMulti = 1000;
            break;
        case 0xA0000000:
            ulMulti = 10000;
            break;
        case 0xB0000000:
            ulMulti = 100000;
            break;
        case 0x20000000:
            ulMulti = 1;
            break;
        default:
            printf("multi=0x%lx", ulMulti);
            assert(0);
    }

    return (ulValue / ulMulti);
}

ULONG FILE_ThsMin2Date(IN ULONG ulMin)
{
    ULONG ulYear, ulMon, ulDate;

    ulYear = ((ulMin & 0xFFF00000) >> 20) + 1900;
    ulMon  =  (ulMin & 0x000F0000) >> 16;
    ulDate =  (ulMin & 0x0000FF00) >> 11;
    return (ulYear * 10000 + ulMon * 100 + ulDate);
}

ULONG FILE_Date2ThsMin(IN ULONG ulDate)
{
    ULONG ulYear, ulMon, ulDay;

    DATE_BREAKDOWN(ulDate,ulYear,ulMon,ulDay);
    assert(ulYear>1900);

    return ((ulYear-1900)<<20) | (ulMon << 16) | (ulDay << 11);
}

//whole data's date must be vaild
ULONG FILE_ThsMin5ToMin30(IN FILE_THS_MIN5_ENTRY_S *pstMin5, OUT FILE_WHOLE_DATA_S *pstWhole)
{
    ULONG i,k;
    ULONG ulMaxPrice, ulMinPrice, ulMin30Vol;
    ULONG ulPrevMin;
    ULONG ulMin5Date;
    PRICE_S *pstMin30Price;

    assert(NULL != pstMin5);
    assert(NULL != pstWhole);

    ulMin5Date = FILE_ThsMin2Date(pstMin5->ulMin);
    assert(ulMin5Date == pstWhole->ulDate);
    pstMin30Price=pstWhole->astMin30Price;
    ulPrevMin = pstMin5->ulMin;
    ulMaxPrice = 0;
    ulMinPrice = 0xFFFFFFFF;
    ulMin30Vol = 0;

    // one day = 4 hours = 48 min5
    for (i=0,k=0;i<48;i++,pstMin5++) {
        if ((pstMin5->ulMin-ulPrevMin) > 720)       //move to next day
            break;
        
        k++;
    
        /* get min&max price in the section */
        if (ulMaxPrice < pstMin5->ulHighWithCheck) {
            ulMaxPrice = pstMin5->ulHighWithCheck;
        }
        if (ulMinPrice > pstMin5->ulLowWithCheck) {
            ulMinPrice = pstMin5->ulLowWithCheck;
        }

        ulMin30Vol += FILE_GetVol(pstMin5->ulVolWithCheck);

        if (1 == k) {
            pstMin30Price->ulBegin = pstMin5->ulBeginWithCheck - FILE_THS_PRICE_CHK;
        }

        /* 30min = 6 * 5min */
        if (6 == k) {
            pstMin30Price->ulHigh = ulMaxPrice - FILE_THS_PRICE_CHK;
            pstMin30Price->ulLow  = ulMinPrice - FILE_THS_PRICE_CHK;
            pstMin30Price->ulEnd  = pstMin5->ulEndWithCheck - FILE_THS_PRICE_CHK;
            pstMin30Price->ulVol  = ulMin30Vol+3; /* each data have 0.5 offset */
            pstMin30Price->ulFlag = FILE_VAILD_PRICE;

            pstMin30Price++;
            ulMin30Vol = 0;
            k = 0;
            ulMaxPrice = 0;
            ulMinPrice = 0xFFFFFFFF;
        }
        ulPrevMin = pstMin5->ulMin;
    }
    return i;
}

VOID FILE_ThsDayToCustom(IN FILE_THS_DAILY_ENTRY_S *pstDay, OUT FILE_WHOLE_DATA_S *pstWhole)
{
    sprintf_s(pstWhole->szComment, 16, "%d", pstDay->ulDate);
    pstWhole->ulDate = pstDay->ulDate;
    pstWhole->ulRsv  = INVAILD_ULONG;
    pstWhole->stDailyPrice.ulBegin = pstDay->ulBeginWithCheck - FILE_THS_PRICE_CHK;
    pstWhole->stDailyPrice.ulHigh  = pstDay->ulHighWithCheck  - FILE_THS_PRICE_CHK;
    pstWhole->stDailyPrice.ulLow   = pstDay->ulLowWithCheck   - FILE_THS_PRICE_CHK;
    pstWhole->stDailyPrice.ulEnd   = pstDay->ulEndWithCheck   - FILE_THS_PRICE_CHK;
    pstWhole->stDailyPrice.ulVol   = FILE_GetVol(pstDay->ulVolWithCheck);
    pstWhole->stDailyPrice.ulFlag  = FILE_VAILD_PRICE;

    return;
}

ULONG FILE_QlWeightToCustom(IN FILE_QL_WEIGHT_ENTRY_S *pstWeight, OUT FILE_WHOLE_DATA_S *pstWhole)
{
    BOOL_T bVaild;
    FLOAT fMultiTemp, fAdderTemp;
    FACTOR_S *pstFactor = &(pstWhole->stFactor);

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
    
    pstFactor->fMulti = fMultiTemp;
    pstFactor->fAdder = fAdderTemp;
    pstFactor->ulFlag = FILE_VAILD_FACTOR;

    return 1;
}

ULONG FILE_QlDate2Custom(IN ULONG ulWeightDate)
{
#define FILE_QL_GET_YEAR(ulDate)    (((ulDate) & 0xFFF00000) >> 20)
#define FILE_QL_GET_MONTH(ulDate)   (((ulDate) & 0x000F0000) >> 16)
#define FILE_QL_GET_DAY(ulDate)     (((ulDate) & 0x0000F800) >> 11)

    ULONG ulYear  = FILE_QL_GET_YEAR(ulWeightDate);
    ULONG ulMonth = FILE_QL_GET_MONTH(ulWeightDate);
    ULONG ulDay   = FILE_QL_GET_DAY(ulWeightDate);

    return ((ulYear * 10000) + (ulMonth * 100) + ulDay);
}


