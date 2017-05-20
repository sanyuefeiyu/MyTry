@echo off
call ndk-build
xcopy "..\..\..\3rd\FFmpeg_3_2_4\android\*.so" ".\libs\armeabi\" /B/Y