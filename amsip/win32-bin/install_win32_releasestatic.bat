
if not exist ..\Release-static mkdir ..\Release-static
if not exist ..\Release-static\plugins mkdir ..\Release-static\plugins
if not exist "..\amsip\platform\vsnet-video\Release\plugins" mkdir "..\amsip\platform\vsnet-video\Release\plugins"

Copy "..\amsiptools\platform\vsnet\Release\amsiptools.dll" ..\Release-static
Copy "..\amsiptools\platform\vsnet\Release\amsiptools.lib" ..\Release-static
Copy "..\amsip\platform\vsnet-video\Release\amsip.lib" ..\Release-static
Copy "..\exosip\platform\vsnet\Release\eXosip.lib" ..\Release-static
Copy "..\osip\platform\vsnet\Release\osip2.lib" ..\Release-static
Copy "..\osip\platform\vsnet\Release\osipparser2.lib" ..\Release-static
Copy "..\oRTP\build\win32native\Release\oRTP.lib" ..\Release-static
Copy "..\oRTP\build\win32native\Release\oRTP.dll" ..\Release-static
Copy "..\mediastreamer2\build\win32native\Release\mediastreamer2.lib" ..\Release-static
Copy "..\mediastreamer2\build\win32native\Release\mediastreamer2.dll" ..\Release-static
Copy "..\plugins\vsnet\video\Release\*.dll" ..\Release-static\plugins
Copy "lib\*.dll" ..\Release-static
Copy "lib\*.lib" ..\Release-static

Copy "..\Release-static\*.dll" "..\amsip\platform\vsnet-video\Release\"
Copy "..\Release-static\plugins\*.dll" "..\amsip\platform\vsnet-video\Release\plugins\"

