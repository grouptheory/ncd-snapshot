#/bin/bash
for f in person-*;
do
	md5sum ${f}/*
done

for d in duration*;
do
	for p in ${d}/person-*;
	do
		md5sum ${p}/*
	done
done
