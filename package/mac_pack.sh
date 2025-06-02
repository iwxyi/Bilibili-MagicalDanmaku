rm -rf "神奇弹幕.app"
rm "神奇弹幕.dmg"

cp -R "../Bilibili-MagicalDanmaku/build/Desktop_Qt_5_14_2_clang_64bit-Release/Bilibili-MagicalDanmaku.app" "神奇弹幕.app"
cp -R "data" "MagicalDanmaku.app/Contents/MacOS/data"
cp -R "../Bilibili-MagicalDanmaku/CHANGELOG.md" "神奇弹幕.app/Contents/MacOS/CHANGELOG.md"
cp -R "../Bilibili-MagicalDanmaku/resources/documents/default_code.json" "神奇弹幕.app/Contents/MacOS/default_code.json"
cp -R "../Bilibili-MagicalDanmaku/www" "神奇弹幕.app/Contents/MacOS/www"

cd /Applications/Qt/5.14.2/clang_64/bin
./macdeployqt ~/Projects/MagicalDanmaku_Archieve/神奇弹幕.app -dmg