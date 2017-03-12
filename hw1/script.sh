
mkdir 315636761
cd 315636761
mkdir temp
cd temp
echo tomer > ./tomer
echo samara > ./samara
echo tomersamara > ./tomersamara
cp  samara ../tomer
cp tomer ../samara
rm tomer
rm samara
mv tomersamara ../
cd ../
rmdir temp
ls -l
echo "The contect of the file samara:"
cat samara
echo "The contect of the file tomer:"
cat tomer
echo "The contect of the file tomersamara:"
cat tomersamara


