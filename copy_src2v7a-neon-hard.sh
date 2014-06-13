mkdir src-v7a-neon-hard
mkdir src-v7a-neon-hard/gp2x
mkdir src-v7a-neon-hard/gp2x/cpuspeed
mkdir src-v7a-neon-hard/gp2x/menu
mkdir src-v7a-neon-hard/include
mkdir src-v7a-neon-hard/m68k
mkdir src-v7a-neon-hard/m68k/fame
mkdir src-v7a-neon-hard/menu
mkdir src-v7a-neon-hard/menu_guichan
mkdir src-v7a-neon-hard/vkbd
cd src-v7a-neon-hard
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
