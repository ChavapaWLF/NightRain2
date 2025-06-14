@echo off
echo 正在下载依赖文件...
mkdir include\stb 2>nul
curl -o include\stb\stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
curl -o include\stb\stb_image_write.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
mkdir textures 2>nul
echo 请手动下载纹理文件并放入textures文件夹
mkdir audio 2>nul
echo 请手动下载音频文件并放入audio文件夹
echo 下载完成!
