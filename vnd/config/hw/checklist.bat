rem # checklist V1.0
adb wait-for-device
adb root
adb remount
rem # driver checklist
echo -------------------------------------tms driver------------------------------------- > output.txt 
adb shell ls -l dev/tms* >> output.txt 



echo -------------------------------------tms driver------------------------------------- >> output.txt 
adb shell find ./sys/ -name '*tms*' >> output.txt 


echo  -------------------------------------debug_level:------------------------------------- >> output.txt 
adb shell cat proc/tmsdev/debug_level  >> output.txt
 

rem # conf checklist
echo  -------------------------------------tms conf------------------------------------- >> output.txt 
adb shell grep -r 'A0, 68, 2A' ./vendor/etc/*.conf  >> output.txt
adb shell grep -r 'A0, 11,' ./vendor/etc/*.conf  >> output.txt


rem # TMS HAL$server checklist
echo -------------------------------------find ./system/ -name '*nfc*'------------------------------------- >> output.txt 
adb shell find ./system/ -name '*nfc*' >> output.txt 



echo  -------------------------------------find ./vendor/ -name '*nfc*'------------------------------------- >> output.txt 
adb shell find ./vendor/ -name '*nfc*' >> output.txt 
 


echo  -------------------------------------grep -r "tms" ./vendor/etc/------------------------------------- >> output.txt 
adb shell grep -r "tms" ./vendor/etc/  >> output.txt



rem # Platform checklist
echo -------------------------------------Platform Information------------------------------------- >> output.txt 
adb shell getprop ro.board.platform	>> output.txt 
echo ------------------------------------- >> output.txt 



echo Android releases: >> output.txt 
adb shell getprop ro.build.version.release	>> output.txt


pause


