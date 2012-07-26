
if not exist ..\Debug-static_novideo mkdir ..\Debug-static_novideo
if not exist ..\Debug-static_novideo\plugins mkdir ..\Debug-static_novideo\plugins
if not exist "..\amsip\platform\vsnet\Debug\plugins" mkdir "..\amsip\platform\vsnet\Debug\plugins"

Copy "..\amsip\platform\vsnet\Debug\amsip.lib" ..\Debug-static_novideo
Copy "..\exosip\platform\vsnet\Debug\eXosip.lib" ..\Debug-static_novideo
Copy "..\osip\platform\vsnet\Debug\osip2.lib" ..\Debug-static_novideo
Copy "..\osip\platform\vsnet\Debug\osipparser2.lib" ..\Debug-static_novideo
Copy "..\oRTP\build\win32native\Debug\oRTP.lib" ..\Debug-static_novideo
Copy "..\oRTP\build\win32native\Debug\oRTP.dll" ..\Debug-static_novideo
Copy "..\mediastreamer2\build\win32-novideo\Debug\mediastreamer2.lib" ..\Debug-static_novideo
Copy "..\mediastreamer2\build\win32-novideo\Debug\mediastreamer2.dll" ..\Debug-static_novideo
Copy "..\plugins\vsnet\novideo\Debug\*.dll" ..\Debug-static_novideo\plugins
Copy "lib\*.dll" ..\Debug-static_novideo
Copy "lib\*.lib" ..\Debug-static_novideo

Copy "..\Debug-static_novideo\*.dll" "..\amsip\platform\vsnet\Debug\"
Copy "..\Debug-static_novideo\plugins\*.dll" "..\amsip\platform\vsnet\Debug\plugins\"

