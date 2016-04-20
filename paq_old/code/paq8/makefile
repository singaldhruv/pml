ALL: paq7asm.asm paq8n.cpp
	#nasm -f elf paq7asm.asm
	g++ paq8n.cpp -DNOASM -dunix -o paq8n paq7asm.o

clean: paq8n paq7asm.o
	rm paq8n paq7asm.o

