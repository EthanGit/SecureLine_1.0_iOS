
if not exist ..\Release_novideo mkdir ..\Release_novideo
if not exist ..\Release_novideo\plugins mkdir ..\Release_novideo\plugins
if not exist "..\amsip\platform\vsnet\Release DLL\plugins" mkdir "..\amsip\platform\vsnet\Release DLL\plugins"

Copy "..\amsip\platform\vsnet\Release DLL\amsip.dll" ..\Release_novideo
Copy "..\amsip\platform\vsnet\Release DLL\amsip.lib" ..\Release_novideo
Copy "..\exosip\platform\vsnet\Release\eXosip.lib" ..\Release_novideo
Copy "..\osip\platform\vsnet\Release\osip2.lib" ..\Release_novideo
Copy "..\osip\platform\vsnet\Release\osipparser2.lib" ..\Release_novideo
Copy "..\oRTP\build\win32native\Release\oRTP.lib" ..\Release_novideo
Copy "..\oRTP\build\win32native\Release\oRTP.dll" ..\Release_novideo
Copy "..\mediastreamer2\build\win32-novideo\Release\mediastreamer2.lib" ..\Release_novideo
Copy "..\mediastreamer2\build\win32-novideo\Release\mediastreamer2.dll" ..\Release_novideo
Copy "..\plugins\vsnet\novideo\Release\*.dll" ..\Release_novideo\plugins
Copy "lib\*.dll" ..\Release_novideo
Copy "lib\*.lib" ..\Release_novideo

Copy "..\Release_novideo\*.dll" "..\amsip\platform\vsnet\Release DLL\"
Copy "..\Release_novideo\plugins\*.dll" "..\amsip\platform\vsnet\Release DLL\plugins\"
