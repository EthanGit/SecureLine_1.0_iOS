
if not exist ..\Debug_novideo mkdir ..\Debug_novideo
if not exist ..\Debug_novideo\plugins mkdir ..\Debug_novideo\plugins
if not exist "..\amsip\platform\vsnet\Debug DLL\plugins" mkdir "..\amsip\platform\vsnet\Debug DLL\plugins"

Copy "..\amsip\platform\vsnet\Debug DLL\amsip.dll" ..\Debug_novideo
Copy "..\amsip\platform\vsnet\Debug DLL\amsip.lib" ..\Debug_novideo
Copy "..\exosip\platform\vsnet\Debug\eXosip.lib" ..\Debug_novideo
Copy "..\osip\platform\vsnet\Debug\osip2.lib" ..\Debug_novideo
Copy "..\osip\platform\vsnet\Debug\osipparser2.lib" ..\Debug_novideo
Copy "..\oRTP\build\win32native\Debug\oRTP.lib" ..\Debug_novideo
Copy "..\oRTP\build\win32native\Debug\oRTP.dll" ..\Debug_novideo
Copy "..\mediastreamer2\build\win32-novideo\Debug\mediastreamer2.lib" ..\Debug_novideo
Copy "..\mediastreamer2\build\win32-novideo\Debug\mediastreamer2.dll" ..\Debug_novideo
Copy "..\plugins\vsnet\novideo\Debug\*.dll" ..\Debug_novideo\plugins
Copy "lib\*.dll" ..\Debug_novideo
Copy "lib\*.lib" ..\Debug_novideo

Copy "..\Debug_novideo\*.dll" "..\amsip\platform\vsnet\Debug DLL\"
Copy "..\Debug_novideo\plugins\*.dll" "..\amsip\platform\vsnet\Debug DLL\plugins\"
