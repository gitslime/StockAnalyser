
; Get stock code list from THS, the first array is count, and the following are code list
Func GetStockList()
   Local $CodeList[3000]
   Local $FILE_StockNameList = "D:\THS\stocknametable.txt"
   Local $fp = FileOpen($FILE_StockNameList, 0)
   
   ; Check if file opened for reading OK
   If $fp = -1 Then
	  MsgBox(0, "Error", "Unable to open file " & $FILE_StockNameList)
	  Exit
   EndIf
   
   ; read line by line
   Local $StockCnt = 0
   While 1
   	  Local $line = FileReadLine($fp)
	  If @error = -1 Then ExitLoop
	  
	  Local $StockCode = StringSplit($line, "|")
	  If IsVaildStock($StockCode[1]) = False Then
		 ;MsgBox(0, "code", "Code"&$StockCode[1]&"invaild")
		 ContinueLoop
	  EndIf
	  $StockCnt+=1
	  $CodeList[$StockCnt]=$StockCode[1]
   WEnd
   
   FileClose($fp)
   $CodeList[0]=$StockCnt
   
   Return $CodeList
EndFunc

Func IsVaildStock($Code)
   If Not StringIsDigit($Code) Then Return False

   If $Code < 010000 Then
	  Return True
   ElseIf 300000 <= $Code And $Code < 310000 Then
	  Return True
   ElseIf 600000 <= $Code And $Code < 610000 Then
	  Return True
   EndIf
   
   Return False
EndFunc
