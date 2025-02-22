@echo off
cl.exe /nologo /EHsc /utf-8 /TC /std:c11 /permissive /W4 kzrjson.c test.c /Fe"test.exe"
del *.obj
test.exe
