#include <stock_list.au3>

Global $OrgData = "F:\StockAnalyser\build\StockAnalyser\Debug\OrgData.exe "
Global $SrcDir  = ' "D:\Program Files (x86)\THS\history"'
;Global $SrcDir  = ' "F:\current"'
Global $TrgDir  = " F:\StockAnalyser\database"

Local $StockList = GetStockList()
Local $i = 1
;MsgBox(0, "debug", "Total CNt="&$StockList[0])
While $i <= $StockList[0]
   Local $StockCode = $StockList[$i]
   
   ;MsgBox(0, "CMD", $OrgData&$StockCode&$SrcDir&$TrgDir)
   Run($OrgData&$StockCode&$SrcDir&$TrgDir,"",@SW_HIDE)
   Sleep(50)
   $i+=1
   
   ; control process count
   Local $Process = ProcessList ("OrgData.exe")
   if $Process[0][0] > 25 Then
	  ;MsgBox(0,"Process List", $Process[0][0]);
	  Sleep(1000)
   EndIf
   
   if WinExists("Microsoft Visual C++ Runtime Library") Or WinExists("OrgData.exe") Then
	  MsgBox(0, "Error", $StockCode)
   EndIf

WEnd

MsgBox(0, "Finish", "update OK")
