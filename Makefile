
all:
	cd CommonLib; gmake
	cd Agent; gmake
	cd Cli; gmake
	
clean:
	cd CommonLib; gmake clean
	cd Agent; gmake clean
	cd Cli; gmake clean
