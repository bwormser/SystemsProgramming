# /*******************************************************************************
#  * Name        : makefile
#  * Author      : Brian Wormser, Michael Karsen
#  * Date        : 4/13/2021
#  * Description : Make file for minishell
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
