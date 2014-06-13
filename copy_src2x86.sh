mkdir src-x86
mkdir src-x86/gp2x
mkdir src-x86/gp2x/cpuspeed
mkdir src-x86/gp2x/menu
mkdir src-x86/include
mkdir src-x86/m68k
mkdir src-x86/m68k/fame
mkdir src-x86/menu
mkdir src-x86/menu_guichan
mkdir src-x86/vkbd
cd src-x86
ln -s ../src/* .
cd gp2x
ln -s ../../src/gp2x/* .
cd cpuspeed
ln -s ../../../src/gp2x/cpuspeed/* .
cd ../menu
ln -s ../../../src/gp2x/menu/* .
cd ../../include
ln -s ../../src/include/* .
cd ../m68k
ln -s ../../src/m68k/* .
cd fame
ln -s ../../../src/m68k/fame/* .
cd ../../menu
ln -s ../../src/menu/* .
cd ../menu_guichan
ln -s ../../src/menu_guichan/* .
cd ../vkbd
ln -s ../../src/vkbd/* .
cd ../..
