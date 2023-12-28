binary=RemarkableLamyEraser/RemarkableLamyEraser
rmdevice?=remarkable.local

.PHONY: clean

${binary}: main.c screenlocations.h
	${CC} -O2 -o ${binary} main.c

deploy: ${binary}
	scp ${binary} root@${rmdevice}:${binary}

clean:
	rm ${binary}
