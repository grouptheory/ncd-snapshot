#/bin/bash
NAME="duration"
SLICES=5

rm -rf duration* person*

#Do the first iteration and create random SEED and SEEDS
./company -c -d ${SLICES}
mkdir ${NAME}0
cp -R person* ${NAME}0
SEED=$(cat SEED)

rm -rf person-*

#Do Second using pre-recorded seeds and seeder file
./company -c -d ${SLICES} -s $SEED -R
mkdir ${NAME}1
cp -R person* ${NAME}1

#compute hashes
echo Computing Hashes
cd ${NAME}0
../makeHash.sh > ../${NAME}0.md5

cd ../${NAME}1
../makeHash.sh > ../${NAME}1.md5

cd ..
echo Compare Runs / There should be no output
diff ${NAME}0.md5 ${NAME}1.md5

rm -rf person*
