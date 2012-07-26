
if not exist ..\Debug mkdir ..\Debug
if not exist ..\Debug\plugins mkdir ..\Debug\plugins
if not exist "..\amsip\platform\vsnet-video\Debug DLL\plugins" mkdir "..\amsip\platform\vsnet-video\Debug DLL\plugins"
if not exist ..\vbamsip\bin mkdir ..\vbamsip\bin
if not exist ..\vbamsip\bin\plugins mkdir ..\vbamsip\bin\plugins

Copy "..\amsiptools\platform\vsnet\Debug\amsiptools.dll" ..\Debug
Copy "..\amsiptools\platform\vsnet\Debug\amsiptools.lib" ..\Debug
Copy "..\amsip\platform\vsnet-video\Debug DLL\amsip.dll" ..\Debug
Copy "..\amsip\platform\vsnet-video\Debug DLL\amsip.lib" ..\Debug
Copy "..\exosip\platform\vsnet\Debug\eXosip.lib" ..\Debug
Copy "..\osip\platform\vsnet\Debug\osip2.lib" ..\Debug
Copy "..\osip\platform\vsnet\Debug\osipparser2.lib" ..\Debug
Copy "..\oRTP\build\win32native\Debug\oRTP.lib" ..\Debug
Copy "..\oRTP\build\win32native\Debug\oRTP.dll" ..\Debug
Copy "..\mediastreamer2\build\win32native\Debug\mediastreamer2.lib" ..\Debug
Copy "..\mediastreamer2\build\win32native\Debug\mediastreamer2.dll" ..\Debug
Copy "..\plugins\vsnet\video\Debug\*.dll" ..\Debug\plugins
Copy "lib\*.dll" ..\Debug
Copy "lib\*.lib" ..\Debug

Copy "..\Debug\*.dll" "..\amsip\platform\vsnet-video\Debug DLL\"
Copy "..\Debug\plugins\*.dll" "..\amsip\platform\vsnet-video\Debug DLL\plugins\"

Copy "..\Debug\*.dll" "..\vbamsip\bin\"
Copy "..\Debug\plugins\*.dll" "..\vbamsip\bin\plugins"

