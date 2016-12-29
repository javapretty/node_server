#! /bin/bash
function read_dir() {
    for file in `ls $1`
    do
        if [ -d $1"/"$file ] 
        then
            read_dir $1"/"$file
        else
	    echo "uglify" $1"/"$file
            ../../UglifyJS2/bin/uglifyjs $1"/"$file -c -m -o $1"/"$file
        fi
    done
}

echo "********uglify js begin********"
read_dir ../js
echo "********uglify js end**********"
