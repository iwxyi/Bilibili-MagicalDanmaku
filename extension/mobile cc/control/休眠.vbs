set ws=createobject("wscript.shell")
ws.run"cmd.exe /c rundll32.exe powrProf.dll,SetSuspendState"