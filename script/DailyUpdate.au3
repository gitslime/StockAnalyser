; auto update daily market price

Local $MainTitle = "同花顺(v8"
Local $DownloadTitle = "数据下载中心"
Local $FinishTitle = ""
Local $LoginTitle = "登录到全部行情主站"
Local $ThsPath = "D:\Program Files (x86)\THS\"

; remove old data
FileDelete($ThsPath&"\history\shase\day\*.day")
FileDelete($ThsPath&"\history\shase\extra\*.ext")
FileDelete($ThsPath&"\history\shase\min5\*.mn5")
FileDelete($ThsPath&"\history\sznse\day\*.day")
FileDelete($ThsPath&"\history\sznse\extra\*.ext")
FileDelete($ThsPath&"\history\sznse\min5\*.mn5")

; check if THS existed
if NOT WinExists($MainTitle) Then
   Run($ThsPath&"hexin.exe")
   WinWaitActive($LoginTitle)
   Send("{ENTER}")
   sleep(30000)
EndIf

; active THS
WinActivate($MainTitle)
WinWaitActive($MainTitle)

WinSetState("", "", @SW_MAXIMIZE)		; max windows

; set enviroment
AutoItSetOption ("MouseCoordMode", 0)    ; relative coords to the active window

; update daily data
MouseClick("primary", 300, 12)
sleep(500)  							; wait for menu speard
Send("{DOWN 7}{ENTER 2}")
sleep(1000)
if WinActive($MainTitle) Then
   Send("{TAB}{ENTER}")
EndIf
WinWaitActive($DownloadTitle)
MouseClick("primary", 96, 116)
sleep(5000)
MouseClick("primary", 56,148)
Sleep(500)
MouseClick("primary", 472, 400)
WinWaitActive($MainTitle)
Send("{ENTER}")
sleep(1000)

; update 5 mins data
MouseClick("primary", 300, 12)
sleep(500)
Send("{DOWN 7}{ENTER 2}")
sleep(1000)
if WinActive($MainTitle) Then
   Send("{TAB}{ENTER}")
EndIf
WinWaitActive($DownloadTitle)
MouseClick("primary", 126, 72)
sleep(1000)
Send("{ENTER}")
WinWaitActive($MainTitle)
Send("{ENTER}")
sleep(1000)

; update to custom database
;Run("e:\stock.bat")
;sleep(1000)
;WinActivate("命令提示符")
;Send("stock.exe{ENTER}")

