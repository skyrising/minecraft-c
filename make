#!/usr/bin/fish

mkdir -p bin

for f in chunk
	echo [C] src/$f.c -\> bin/$f.o
	gcc -O3 -c src/$f.c -o bin/$f.o
end

echo [MAIN] src/main.c -\> main
gcc -O3 bin/*.o src/main.c -o main
echo [MAIN] src/check_iron.c -\> check_iron
gcc -O3 bin/*.o src/check_iron.c -o check_iron
