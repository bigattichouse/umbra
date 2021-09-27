gcc pages/addressBookPages.c addressBook.c -o addressBook -ldl

pushd pages/
gcc -ldl -o ../bin/addressBookData.0.so addressBookData.0.c  -shared -fPIC
gcc -ldl -o ../bin/addressBookData.1.so addressBookData.1.c  -shared -fPIC

popd
