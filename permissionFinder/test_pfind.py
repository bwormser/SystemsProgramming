"""
WARNING: It is highly suggested you run the test script in a separate directory from any other project.

In order to test your code, unzip the file "03-permission-find-packed-dir.tar.gz" with the command:
    sudo tar -xzvf 03-permission-find-packed-dir.tar.gz 

Usage:
    "python test.py" -> assuming Makefile and all necessary .c and .h files are in current directory,
    attempts to make project and run tests.

THIS TEST SCRIPT DOES NOT REFLECT YOUR FINAL GRADE (WE USE A BIGGER TEST SCRIP FOR GRADING)

Last Updated: March 26, 2020
"""

from random import randrange
import os
import subprocess
import time
import sys
import signal
import re
import zipfile
import difflib
import shutil

FILE = "./pfind"
TIMEOUT = 1
RANDOM = True # set to false to disable random testing
TEST_DIR_PATH = "./03-permission-find-packed-dir"
PATH_TO_ZIPS = "./temp"
PATH_MAX = 4096
FILE_NAME_MAX = 255

ABS_TEST_DIR = os.path.abspath(TEST_DIR_PATH)

class bcolors:
    yellow = '\033[93m'
    end = '\033[0m'
    red = '\033[91m'
    green = '\033[92m'

test_count = 1
correct_count = 0
total_points = 0
actual_points = 0
valgrind_test_count = 1

valgrind = 0
makefile = 0

def color_str(string, color):
    return color + string + bcolors.end

def create_file(name, text):
    file = open(name, 'w')
    file.write(text)
    file.close()
    return name

'''
Tests program.
    args : Array of strings given to progam as command line args
    expected : The string representation of output printed to stdout 
    and stderr after completion of program excecutation.
    returncode (optional) : Int. Non-zero if expecting non-zero exit code, zero otherwise.
    value (optional) : How much the test is worth.
    fail_message (optional) : Custom message printed if test fails, override default printing.
    file (optional) : The name of the executable to be run.
'''
def test_args(args, expected, returncode=0, value=1, fail_message="", file=FILE, strict_output=False, strict_code=False):
    global total_points
    global actual_points
    global test_count
    global correct_count

    start_time = time.time()
    try:
        result = subprocess.run([file] + args, timeout=TIMEOUT, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired: #Handle timeout error
        print(color_str("--------------------------------------------------------------------------------", bcolors.red))
        print("Running test {}...".format(test_count)  + color_str("failure", bcolors.red) 
                + " - TIMEOUT")
        print(color_str("--------------------------------------------------------------------------------", bcolors.red))
        test_count += 1
        return

    elapsed_time = time.time() - start_time
    total_points += value

    #removes trailing \n 
    output = result.stdout
    if (output[-1:] == '\n'):
        output = output[:-1]
    #Check returncode
    actual_return = result.returncode
    return_check = ((returncode == 0) and (actual_return == 0)) or ((returncode != 0) and (actual_return != 0)) if strict_code else True
    seg_check = (actual_return == -signal.SIGSEGV)
    if output == expected and return_check:
        actual_points += value
        correct_count += 1
        print("Running test {}...".format(test_count) + color_str("success", bcolors.green) + 
            " - Elapsed time: " + "{:.3f} seconds".format(elapsed_time))
    else:
        print(color_str("--------------------------------------------------------------------------------", bcolors.red))
        print("Running test {}...".format(test_count)  + color_str("failure", bcolors.red) 
                + " - Elapsed time: " + "{:.3f} seconds".format(elapsed_time))
        if fail_message == "" and not seg_check:
            print(color_str("Expected:", bcolors.yellow) + " " + str(returncode))
            print(expected)
            print(color_str("Received:", bcolors.yellow) + " " + str(actual_return))
            print(output)    

            if (strict_output):
                print()
                diff_list = []
                for i,s in enumerate(difflib.ndiff(output, expected)):
                    if s[0]==' ': continue
                    elif s[0]=='-':
                        diff_list.append(color_str(u'Delete "{}" from position {}'.format(s[-1],i), bcolors.yellow))
                    elif s[0]=='+':
                        diff_list.append(color_str(u'Add "{}" to position {}'.format(s[-1],i), bcolors.yellow))
                if len(diff_list) < 10:
                    for s in diff_list:
                        print(s)
                else:
                    print(color_str('Differences should be obvious (greater then 10 chars).', bcolors.yellow))
        elif seg_check:
            print(color_str("Segmentation Fault (core dumped)", bcolors.red))
        else:
            print(fail_message)
        print(color_str("--------------------------------------------------------------------------------", bcolors.red))
    
    test_count += 1

def valgrind_test(args):
    global total_points
    global actual_points
    global valgrind_test_count
    global correct_count

    global valgrind
    global makefile

    start_time = time.time()
    try:
        result = subprocess.run(["valgrind"] + [FILE] + args, timeout=5, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        print(color_str("--------------------------------------------------------------------------------", bcolors.red))
        print("Running valgrind test {}...".format(valgrind_test_count)  + color_str("failure", bcolors.red) 
                + " - Elapsed time:" + color_str(" TIMEOUT", bcolors.red))
        print(color_str("--------------------------------------------------------------------------------", bcolors.red))
        valgrind = 5
        valgrind_test_count += 1
        return
    elapsed_time = time.time() - start_time


    output = result.stdout
    if ("All heap blocks were freed -- no leaks are possible" in output):
        print("Running valgrind test {}...".format(valgrind_test_count) + color_str("success", bcolors.green) + 
            " - Elapsed time: " + "{:.3f} seconds".format(elapsed_time))
    else:
        print(color_str("--------------------------------------------------------------------------------", bcolors.red))
        print("Running valgrind test {}...".format(valgrind_test_count)  + color_str("failure", bcolors.red) 
                + " - Elapsed time: " + "{:.3f} seconds".format(elapsed_time))
        start = output.find("LEAK SUMMARY:")
        end = output.find("Rerun with")
        print(output[start:end])
        print(color_str("--------------------------------------------------------------------------------", bcolors.red))
        if valgrind == 0:
            valgrind = 5
    valgrind_test_count += 1

def makefile_test():
    global makefile

    makefile_dir = ''

    if os.path.exists('makefile'):
        makefile_dir = 'makefile'
    elif os.path.exists('Makefile'):
        makefile_dir = 'Makefile'
    else:
        print(color_str("Error: ", bcolors.red) + "Cannot find makefile: -10\n")
        makefile = 10
        generate_makefile()

    result = subprocess.run(["make"], timeout=TIMEOUT, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode == 2:
        print(color_str("Error: ", bcolors.red) + "Supplied makefile did not work or compilation error:" + FILE + ": -10\n")
        makefile = 10
    else:
        with open(makefile_dir) as f:
            string = f.read().lower()
            if '-wall' in string and '-werror' in string and '-pedantic-errors' in string:
                pass
            else: 
                print(color_str("Error: ", bcolors.red) + "Makefile missing wall, werror or pedantic-errors: -5.\n")
                makefile = 5


def illegal_call_test(files, strings):
    for file in files:
        with open (file) as opened:
            file_contents = opened.read()
            for string in strings:
                if string in file_contents:
                    print(color_str("Error: ", bcolors.red) + "'" + string + "' found in file. \nPercent Correct: 0")
                    sys.exit(0)

#############################################
# Unzipping / mv
#############################################

# Check if test directory exists
if (not os.path.isdir(TEST_DIR_PATH)):
    print(color_str("Failed: ", bcolors.red) + " Could not find test directory " + color_str(TEST_DIR_PATH, bcolors.yellow))
    sys.exit(1)

if (not len(sys.argv) == 1):
    print(color_str("Expect:", bcolors.red) + " ./test.py (just run the thing)")
    sys.exit(1)    

usage = "Usage: ./pfind -d <directory> -p <permissions string> [-h]"

#############################################
# Initial tests
#############################################

makefile_test()

#Check if FILE exists
if not os.path.isfile(FILE):
    print("Could not find " + "'" + FILE + "'")
    sys.exit(0) 

#############################################
# Begin Tests
#############################################
print(color_str(".....................Beginning tests...................", bcolors.yellow))
# Test options
test_args([], usage, returncode=1, strict_output=True)
test_args(['-h'], usage, returncode=1, strict_output=True)
test_args(['-p'], "Error: Required argument -d <directory> not found.", returncode=1, strict_output=True)
test_args(['-d', 'something', '-p'], "Error: Required argument -p <permissions string> not found.", returncode=1, strict_output=True)
test_args(['-d', 'something', '-p', '---------'], "Error: Cannot stat 'something'. No such file or directory.", returncode=1, strict_output=True)
test_args(['-d', TEST_DIR_PATH, '-p', '-wx'], "Error: Permissions string '-wx' is invalid.", returncode=1, strict_output=True)
test_args(['-d', TEST_DIR_PATH, '-p', '--------'], "Error: Permissions string '--------' is invalid.", returncode=1, strict_output=True)
test_args(['-d', TEST_DIR_PATH + '/cars/fast', '-p', '---------'], 
"Error: Cannot open directory '" + ABS_TEST_DIR + "/cars/fast'. Permission denied.",
returncode=1,
strict_output=True)

# Test basic behavior
test_args(['-d', TEST_DIR_PATH, '-p', 'rwxrwxrwx'],
"Error: Cannot open directory '" + ABS_TEST_DIR + "/cars/fast'. Permission denied.\n" +
ABS_TEST_DIR + "/fruit/bad/starfruit\n" +
ABS_TEST_DIR + "/fruit/good/sl_to_base")


test_args(['-d', TEST_DIR_PATH, '-p', 'rwxr-xr-x'],
"Error: Cannot open directory '" + ABS_TEST_DIR + "/cars/fast'. Permission denied.\n" +
ABS_TEST_DIR + "/fruit\n" +
ABS_TEST_DIR + "/fruit/bad\n" +
ABS_TEST_DIR + "/fruit/good\n" +
ABS_TEST_DIR + "/cars"
)


correct = ((actual_points / total_points) * 100) - makefile - valgrind

print("\nTotal tests run: {}".format(test_count - 1))
print("Number correct : {}".format(correct_count))
print("Percent correct: {:.2f}".format(correct))


subprocess.run(["make", "clean"], timeout=TIMEOUT, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
