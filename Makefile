binary=RMStylusButton/RMStylusButton
tarball=RMStylusButton.tar.gz
rmdevice?=remarkable.local

.PHONY: clean

${binary}: main.c
	${CC} -O2 -o ${binary} main.c

${tarball}: ${binary} RMStylusButton/manage.sh
	tar czf ${tarball} RMStylusButton/

deploy: ${tarball}
	scp ${tarball} root@${rmdevice}:${tarball}

clean:
	rm -f ${binary} ${tarball}
