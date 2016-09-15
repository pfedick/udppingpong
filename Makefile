
all:
	cd CommonLib; make
	cd Agent; make
	cd Cli; make
	
clean:
	cd CommonLib; make clean
	cd Agent; make clean
	cd Cli; make clean
