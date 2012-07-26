
if not exist ..\Release mkdir ..\Release
if not exist ..\Release\plugins mkdir ..\Release\plugins
if not exist "..\amsip\platform\vsnet-video\Release DLL\plugins" mkdir "..\amsip\platform\vsnet-video\Release DLL\plugins"
if not exist ..\vbamsip\bin mkdir ..\vbamsip\bin
if not exist ..\vbamsip\bin\plugins mkdir ..\vbamsip\bin\plugins

Copy "..\amsiptools\platform\vsnet\Release\amsiptools.dll" ..\Release
Copy "..\amsiptools\platform\vsnet\Release\amsiptools.lib" ..\Release
Copy "..\amsip\platform\vsnet-video\Release DLL\amsip.dll" ..\Release
Copy "..\amsip\platform\vsnet-video\Release DLL\amsip.lib" ..\Release
Copy "..\exosip\platform\vsnet\Release\eXosip.lib" ..\Release
Copy "..\osip\platform\vsnet\Release\osip2.lib" ..\Release
Copy "..\osip\platform\vsnet\Release\osipparser2.lib" ..\Release
Copy "..\oRTP\build\win32native\Release\oRTP.lib" ..\Release
Copy "..\oRTP\build\win32native\Release\oRTP.dll" ..\Release
Copy "..\mediastreamer2\build\win32native\Release\mediastreamer2.lib" ..\Release
Copy "..\mediastreamer2\build\win32native\Release\mediastreamer2.dll" ..\Release
Copy "..\plugins\vsnet\video\Release\*.dll" ..\Release\plugins
Copy "lib\*.dll" ..\Release
Copy "lib\*.lib" ..\Release

Copy "..\Release\*.dll" "..\amsip\platform\vsnet-video\Release DLL\"
Copy "..\Release\plugins\*.dll" "..\amsip\platform\vsnet-video\Release DLL\plugins\"

Copy "..\Release\*.dll" "..\vbamsip\bin\"
Copy "..\Release\plugins\*.dll" "..\vbamsip\bin\plugins"
