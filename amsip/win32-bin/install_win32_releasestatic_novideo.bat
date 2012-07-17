
if not exist ..\Release-static_novideo mkdir ..\Release-static_novideo
if not exist ..\Release-static_novideo\plugins mkdir ..\Release-static_novideo\plugins
if not exist "..\amsip\platform\vsnet\Release\plugins" mkdir "..\amsip\platform\vsnet\Release\plugins"

Copy "..\amsip\platform\vsnet\Release\amsip.lib" ..\Release-static_novideo
Copy "..\exosip\platform\vsnet\Release\eXosip.lib" ..\Release-static_novideo
Copy "..\osip\platform\vsnet\Release\osip2.lib" ..\Release-static_novideo
Copy "..\osip\platform\vsnet\Release\osipparser2.lib" ..\Release-static_novideo
Copy "..\oRTP\build\win32native\Release\oRTP.lib" ..\Release-static_novideo
Copy "..\oRTP\build\win32native\Release\oRTP.dll" ..\Release-static_novideo
Copy "..\mediastreamer2\build\win32-novideo\Release\mediastreamer2.lib" ..\Release-static_novideo
Copy "..\mediastreamer2\build\win32-novideo\Release\mediastreamer2.dll" ..\Release-static_novideo
Copy "..\plugins\vsnet\novideo\Release\*.dll" ..\Release-static_novideo\plugins
Copy "lib\*.dll" ..\Release-static_novideo
Copy "lib\*.lib" ..\Release-static_novideo

Copy "..\Release-static_novideo\*.dll" "..\amsip\platform\vsnet\Release\"
Copy "..\Release-static_novideo\plugins\*.dll" "..\amsip\platform\vsnet\Release\plugins\"

