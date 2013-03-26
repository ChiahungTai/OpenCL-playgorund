#Script to modify the aparapi.jar path in .classpath file(aparapi/examples) with LIBAPARAPI environment variable.


ClassFiles=`ls -ad $(find .)|grep "\.classpath"|grep -v "\.classpath.bak"`
#AparapiInFile="C:/dev/aparapi/aparapi-2012-05-06/aparapi.jar";
PATH_LIBAPARAPI=`echo $LIBAPARAPI`;

if [ -z "$PATH_LIBAPARAPI" ]
then
echo "LIBAPARAPI PATH NOT SET : export LIBAPARAPI='enter path to aparapi.jar'";
exit;
fi


for file in $ClassFiles;
do
if [ -f $file ]
then
	if [ -f $file.bak ]
	then
	mv $file.bak $file;
	fi
echo "Processing File..$file"
#Assign current path in .classpath here
perl -pi.bak -e 's/C:\/dev\/aparapi\/aparapi-2012-05-06/$ENV{LIBAPARAPI}/' "$file";
echo "Modified $file (Created backup file with .classpath.bak)";
fi

done
echo "LIBAPARAPI PATH : $PATH_LIBAPARAPI";
