if not exist ..\include\amsip mkdir ..\include\amsip
Copy ..\amsip\include\amsip\*.h ..\include\amsip\

if not exist ..\include\amsiptools mkdir ..\include\amsiptools
Copy ..\amsiptools\include\amsiptools\*.h ..\include\amsiptools\

if not exist ..\include\ppl mkdir ..\include\ppl
Copy ..\amsip\ppl\win32\ppl\*.h ..\include\ppl\

if not exist ..\include\eXosip2 mkdir ..\include\eXosip2
Copy ..\exosip\include\eXosip2\*.h ..\include\eXosip2\

if not exist ..\include\osip2 mkdir ..\include\osip2
if not exist ..\include\osipparser2 mkdir ..\include\osipparser2
if not exist ..\include\osipparser2\headers mkdir ..\include\osipparser2\headers
Copy ..\osip\include\osip2\*.h ..\include\osip2\
Copy ..\osip\include\osipparser2\*.h ..\include\osipparser2\
Copy ..\osip\include\osipparser2\headers\*.h ..\include\osipparser2\headers

if not exist ..\include\ortp mkdir ..\include\ortp
Copy ..\oRTP\include\ortp\*.h ..\include\ortp\

if not exist ..\include\mediastreamer2 mkdir ..\include\mediastreamer2
Copy ..\mediastreamer2\include\mediastreamer2\*.h ..\include\mediastreamer2\

if not exist ..\include\srtp mkdir ..\include\srtp
Copy ..\win32-bin\include\srtp\*.h ..\include\srtp\

if not exist ..\include\speex mkdir ..\include\speex
Copy ..\codecs\speex\include\speex\*.h ..\include\speex
