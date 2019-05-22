PROJECT_SRC = feature_extraction_sdhash.cpp util_sql.c


NAME=f_extractor_sdhash

all: debug

debug: ${PROJECT_SRC} ${PROJECT_HDR}
	g++ -w -ggdb -std=c99 -D_BSD_SOURCE -o ${NAME} ${PROJECT_SRC} -Dnetwork -lm -lssl -lcrypto -l sqlite3 -DTHREADSAFE=1 -lpthread

mrsh: ${PROJECT_SRC} ${PROJECT_HDR}
	g++ -w -std=c99 -O3 -D_BSD_SOURCE -o ${NAME} ${PROJECT_SRC} -lm -lssl -lcrypto -l sqlite3 -DTHREADSAFE=1 -lpthread

net: ${PROJECT_SRC} ${PROJECT_HDR}
	g++ -w -std=c99 -O3 -D_BSD_SOURCE -o ${NAME} ${PROJECT_SRC} -Dnetwork -lm -lssl -lcrypto -l sqlite3 -DTHREADSAFE=1 -lpthread
