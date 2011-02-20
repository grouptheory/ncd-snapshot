#/bin/bash
NAME="testseed"

rm -rf duration* person* testseed* SEED* *.run

#Do the first iteration and create random SEED and SEEDS
./makeImages.sh &> test1.run
mkdir ${NAME}0
cp -R person* ${NAME}0
cp -R duration* ${NAME}0

rm -rf duration* person*

#Do Second using pre-recorded seeds and seeder file
./makeImages-reply.sh &> test2.run
mkdir ${NAME}1
cp -R person* ${NAME}1
cp -R duration* ${NAME}1

#compute hashes
echo Computing Hashes
cd ${NAME}0
../makeHash.sh > ../${NAME}0.md5

cd ../${NAME}1
../makeHash.sh > ../${NAME}1.md5

cd ..
echo Compare Runs / There should be no output
diff ${NAME}0.md5 ${NAME}1.md5

rm -rf person* duration*
