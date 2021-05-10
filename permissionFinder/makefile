# /*******************************************************************************
#  * Name        : pfind.c
#  * Author      : Brian Wormser, Michael Karsen
#  * Date        : 3/16/2021
#  * Description : Recursively finds directories and files with matching permissions.
#  * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
#  ******************************************************************************/

CC     = gcc
C_FILE = $(wildcard *.c)
TARGET = $(patsubst %.c,%,$(C_FILE))
CFLAGS = -g -Wall -Werror -pedantic-errors

all:
	$(CC) $(CFLAGS) $(C_FILE) -o $(TARGET)
clean:
	rm -f $(TARGET) $(TARGET).exe
