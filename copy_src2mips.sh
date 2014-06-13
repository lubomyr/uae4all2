mkdir src-mips
mkdir src-mips/gp2x
mkdir src-mips/gp2x/cpuspeed
mkdir src-mips/gp2x/menu
mkdir src-mips/include
mkdir src-mips/m68k
mkdir src-mips/m68k/fame
mkdir src-mips/menu
mkdir src-mips/menu_guichan
mkdir src-mips/vkbd
cd src-mips
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
