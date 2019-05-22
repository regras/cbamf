PROJECT_SRC = ./src/main.c ./src/util.c src/util_sql.c src/hashing.c src/bloomfilter.c src/fingerprint.c src/fingerprintList.c src/helper.c

NAME=mrsh

all: debug

debug: ${PROJECT_SRC} ${PROJECT_HDR}
	gcc -w -ggdb -std=c99 -D_BSD_SOURCE -lcrypto -o ${NAME} ${PROJECT_SRC} -Dnetwork -lm -l sqlite3

mrsh: ${PROJECT_SRC} ${PROJECT_HDR}
	gcc -w -std=c99 -O3 -D_BSD_SOURCE -lcrypto -o ${NAME} ${PROJECT_SRC} -lm -l sqlite3

net: ${PROJECT_SRC} ${PROJECT_HDR}
	gcc -w -std=c99 -O3 -D_BSD_SOURCE -lcrypto -o ${NAME} ${PROJECT_SRC} -Dnetwork -lm -l sqlite3

clean :  
	rm -f mrsh *.o 



