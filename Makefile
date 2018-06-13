all:
	cd CommonLib; make
	cd UdpPingPong; make
	cd Agent; make
	cd Cli; make
	
clean:
	cd CommonLib; make clean
	cd UdpPingPong; make clean
	cd Agent; make clean
	cd Cli; make clean
