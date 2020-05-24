myfind: myfind.o
		gcc -g myfind.o -o myfind
		
myfind.o: myfind.c
		gcc -g -Wall -c myfind.c 
		
clean: 
		rm *.o myfind
		
		
