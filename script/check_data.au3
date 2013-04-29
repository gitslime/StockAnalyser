#include <Constants.au3>
#include <stock_list.au3>

Global $OrgData = "F:\StockAnalyser\build\StockAnalyser\Debug\OrgData.exe "
Global $ChkData = "F:\StockAnalyser\build\StockAnalyser\Debug\CheckData.exe "
Global $SrcDir  = ' "D:\Program Files (x86)\THS\history"'
Global $WgtDir  = ' "D:\Program Files (x86)\QianLong\qijian\QLDATA\history" '
;Global $SrcDir  = ' "F:\current"'
Global $TrgDir  = " F:\StockAnalyser\database"

Local $StockList = GetStockList()
Local $i = 1

While $i <= $StockList[0]
   Local $IsMiss = False
   Local $StockCode = $StockList[$i]
   If $StockCode < 600589 Then
	  $i+=1
	  ContinueLoop
   EndIf
   
   Local $foo = Run($ChkData&$TrgDir&$WgtDir&$StockCode, "", @SW_HIDE, $STDOUT_CHILD)
   ;Sleep(1000)
   
   Local $line
   While 1
	  $line = StdoutRead($foo)
	  If @error Then ExitLoop
	  
	 ;MsgBox(0, "READ", $line)
	  If StringInStr($line, "miss") Then
		 $IsMiss = True
		 ;MsgBox(0, "data miss", $StockCode&@CRLF&$line)
	  EndIf
   WEnd
   
   If Not $IsMiss Then 
	  MsgBox(0, "", $StockCode&"OK", 1)
	  $i+=1
	  ContinueLoop
   EndIf
   
   AutoItSetOption ("SendKeyDelay", 100) ;5 milliseconds
   WinActivate("Í¬»¨Ë³(v8.20.74)")
   ;Sleep(1000)
   Send($StockCode&"{ENTER}")
   ;MsgBox(0, "", $StockCode&"OK")
   MouseClick("primary", 147, 47)
   Sleep(500)
   Send("{DOWN 2}")
   Local $k = 0
   While $k < 2
	  Send("{DOWN 4}")
	  Sleep(2500)
	  ;Send("{LEFT}{HOME}{LEFT 2}")
	  Send("{UP 7}")
	  Sleep(500)
	  Send("{DOWN 8}")
	  $k+=1
   WEnd
   Sleep(1000)
 
   ;MsgBox(0, "", $StockCode&"OK")
   
   ;MsgBox(0, "CMD", $OrgData&$StockCode&$SrcDir&$TrgDir)
   Run($OrgData&$StockCode&$SrcDir&$TrgDir,"",@SW_HIDE)
   Sleep(50)
   $i+=1
   ;MsgBox(0, "", $StockCode&" Checked!")
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