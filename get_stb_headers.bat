@echo off
mkdir include\stb 2>nul
curl -o include\stb\stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
curl -o include\stb\stb_image_write.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
echo STB headers downloaded successfully!