CC := arm-linux-gnueabihf-gcc

CFLAGS := -g -Wall -I./
LDFLAGS := -lpthread

LIBS	= 

TARGET = u2_demo

OBJS = u2_demo.o \
	i2c_common.o \
	ht16k33_common.o \
	pca9685.o\
	i2c_s1_key.o\
	e1.o\
	e2.o\
	e3.o\
	s6.o\
	c3.o
SRC = $(OBJS:%.o=%.c)

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS) ${LIBS}

$(OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o $(TARGET) 
