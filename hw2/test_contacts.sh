#!/bin/bash

# file=$(find . -iregex '.*sort.c')
file="./sort.c"
file=${file:2} # Strip away leading ./

if [ ! -f "$file" ]; then
    echo $file
    echo -e "Error: File '$file' not found.\nTest failed."
    exit 1
fi

num_right=0
total=0
line="________________________________________________________________________"
compiler=
interpreter=
language=
extension=${file##*.}
if [ "$extension" = "c" ]; then
    language="c"
    command="./${file%.*}"
    echo -n "Compiling $file..."
    results=$(make 2>&1)
    if [ $? -ne 0 ]; then
        echo -e "\n$results"
        exit 1
    fi
    echo -e "done\n"
fi

run_test() {
    (( ++total ))
    echo -n "Running test $((total/2 +1))..."
    expected=$2
    received=$( $command $1 2>&1 | tr -d '\r' )
    if [ "$expected" = "$received" ]; then
        echo "success"
        (( ++num_right ))
    else
        echo -e "failure\n\nExpected$line\n$expected\nReceived$line\n$received\n"
    fi
    valgrind_checker "$1"
}

valgrind_checker() {
    (( ++total ))
    echo -n "Running Valgrind on test $((total/2))..."
    valgrind --leak-check=full --error-exitcode=127 $command $1 > out.txt 2>&1
    if [ $? -eq 127 ]; then
        echo "Failed :("
    else
        echo "Passed!"
        (( ++num_right ))
    fi
    rm -f out.txt
}

run_test "$(cat "cases/input1.txt")" "$(cat "cases/output1.txt")"
run_test "$(cat "cases/input2.txt")" "$(cat "cases/output2.txt")"
run_test "$(cat "cases/input3.txt")" "$(cat "cases/output3.txt")"
run_test "$(cat "cases/input4.txt")" "$(cat "cases/output4.txt")"
run_test "$(cat "cases/input5.txt")" "$(cat "cases/output5.txt")"
run_test "$(cat "cases/input6.txt")" "$(cat "cases/output6.txt")"
run_test "$(cat "cases/input7.txt")" "$(cat "cases/output7.txt")"

(cat << EOF
123
456
7
9
23456
5647657
3244
435465567
EOF
) > input.txt

run_test "$(cat "cases/input8.txt")" "$(cat "cases/output8.txt")"
rm -f input.txt

(cat << EOF
12.5
1003.345
5677
2424.2424
800
800.0001
12.50
EOF
) > input.txt

run_test "$(cat "cases/input9.txt")" "$(cat "cases/output9.txt")"
rm -f input.txt

(cat << EOF
rocco
marco
code
quick
explore
value
strings are equal
EOF
) > input.txt

run_test "$(cat "cases/input10.txt")" "$(cat "cases/output10.txt")"
rm -f input.txt

(cat << EOF
4
2
3
1
EOF
) > input.txt

run_test "$(cat "cases/input11.txt")" "$(cat "cases/output11.txt")"
rm -f input.txt

echo -e "\nTotal tests run: $total"
echo -e "Number correct : $num_right"
echo -n "Percent correct: "
echo "scale=2; 100 * $num_right / $total" | bc

if [ "$language" = "java" ]; then
   echo -e -n "\nRemoving class files..."
   rm -f *.class
   echo "done"
elif [ "$language" = "c" ]; then
    echo -e -n "\nCleaning project..."
    make clean > /dev/null 2>&1
    echo "done"
fi
