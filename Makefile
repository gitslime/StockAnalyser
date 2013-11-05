
CC=gcc
CFLAGS=-Wall -DLINUX

all: check choose org show sim stat

METHOD_DIR=./method
METHOD_SRC=$(METHOD_DIR)/method.c $(METHOD_DIR)/mma.c $(METHOD_DIR)/oma.c $(METHOD_DIR)/rise.c $(METHOD_DIR)/sma.c 
METHOD_OBJ=$(METHOD_SRC:.c=.o) 

$(METHOD_OBJ): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

COMM_DIR=./common
COMM_SRC=$(COMM_DIR)/file.c $(COMM_DIR)/stocklist.c $(COMM_DIR)/stockutil.c $(COMM_DIR)/util.c
COMM_OBJ=$(COMM_SRC:.c=.o) 

$(COMM_OBJ): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	rm -f $(METHOD_DIR)/*.o
	rm -f $(COMM_DIR)/*.o
	rm -f check* choose* org* show* sim* stat* 
		
check : $(COMM_OBJ) CheckData.o 
	$(CC) $(CFLAGS) -o $@ CheckData.o $(COMM_OBJ)

choose : $(COMM_OBJ) $(METHOD_OBJ) Choose.o
	$(CC) $(CFLAGS) -o $@ Choose.o $(COMM_OBJ) $(METHOD_OBJ)
	
org : $(COMM_OBJ) OrgData.o
	$(CC) $(CFLAGS) -o $@ OrgData.o $(COMM_OBJ)

show : $(COMM_OBJ) ShowData.o
	$(CC) $(CFLAGS) -o $@ ShowData.o $(COMM_OBJ)

sim : $(COMM_OBJ) $(METHOD_OBJ) Simulate.o
	$(CC) $(CFLAGS) -o $@ Simulate.o $(COMM_OBJ) $(METHOD_OBJ)

stat : $(COMM_OBJ) $(METHOD_OBJ) Statistics.o
	$(CC) $(CFLAGS) -o $@ Statistics.o $(COMM_OBJ) $(METHOD_OBJ)	
	
