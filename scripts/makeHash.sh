#/bin/bash
for f in person-*;
do
	md5sum ${f}/*
done
