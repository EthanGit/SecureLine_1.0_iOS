
if not exist ..\Debug-static mkdir ..\Debug-static
if not exist ..\Debug-static\plugins mkdir ..\Debug-static\plugins
if not exist "..\amsip\platform\vsnet-video\Debug\plugins" mkdir "..\amsip\platform\vsnet-video\Debug\plugins"

Copy "..\amsiptools\platform\vsnet\Debug\amsiptools.dll" ..\Debug-static
Copy "..\amsiptools\platform\vsnet\Debug\amsiptools.lib" ..\Debug-static
Copy "..\amsip\platform\vsnet-video\Debug\amsip.lib" ..\Debug-static
Copy "..\exosip\platform\vsnet\Debug\eXosip.lib" ..\Debug-static
Copy "..\osip\platform\vsnet\Debug\osip2.lib" ..\Debug-static
Copy "..\osip\platform\vsnet\Debug\osipparser2.lib" ..\Debug-static
Copy "..\oRTP\build\win32native\Debug\oRTP.lib" ..\Debug-static
Copy "..\oRTP\build\win32native\Debug\oRTP.dll" ..\Debug-static
Copy "..\mediastreamer2\build\win32native\Debug\mediastreamer2.lib" ..\Debug-static
Copy "..\mediastreamer2\build\win32native\Debug\mediastreamer2.dll" ..\Debug-static
Copy "..\plugins\vsnet\video\Debug\*.dll" ..\Debug-static\plugins
Copy "lib\*.dll" ..\Debug-static
Copy "lib\*.lib" ..\Debug-static

Copy "..\Debug-static\*.dll" "..\amsip\platform\vsnet-video\Debug\"
Copy "..\Debug-static\plugins\*.dll" "..\amsip\platform\vsnet-video\Debug\plugins\"

