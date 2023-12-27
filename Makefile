binary=RemarkableLamyEraser/RemarkableLamyEraser

.PHONY: clean

${binary}: main.c screenlocations.h
	${CC} -O2 -o ${binary} main.c

clean:
	rm ${binary}
