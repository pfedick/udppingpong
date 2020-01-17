# UDPPingPong
Tools for measuring udp throughput:
- pingpong_sender: generates packets of arbitrary size and amount and counts
  the answers and throughput
- pingpong_bouncer: recieves the packets on the other side and sends them back.


# Build and install
### pre requirements
- decent compiler which is capable of compiling stdc++11 code, e.g. gcc >= 4.6 or clang >= 3.3
- pcre library (developer packages)

### compile and install 
    ./configure
    make
    make install

By default the binaries will be installed in /usr/local/bin. If you want them
somewhere else (e.g. /usr/bin), please use "./configure --prefix=xxxx", where "xxxx" is
the destination path:

    ./configure --prefix=/usr

### Uninstall
    make uninstall


# Usage

Let's assume we have 2 machines with these IPs:
- machine1: 192.168.0.1 (=sender)
- machine2: 192.168.0.2 (=receiver)

Start bouncer on destination machine2 and bind it to the interface and port on which the packets will be
received:

    pingpong_bouncer -s 192.168.0.2:5000 -n 2
    
Now start the sender on machine1:

    pingpong_sender -z 192.168.0.2:5000 -r 100000 -n 2 -p 256
    
This command will send 100000 packages per second with 2 sender threads and a packet size of 256 bytes for
10 seconds. The receiver (bouncer) also listenes with 2 worker threads and will bounce the packets back to the sender.

On the receiver you will get some statistics showing packets received, packets send and throughput:
 
	Packets per second:          0, Durchsatz:          0 Mbit, RX:        4, TX:        1
	Packets per second:          0, Durchsatz:          0 Mbit, RX:        4, TX:        1
	Packets per second:      46300, Durchsatz:         90 Mbit, RX:    46405, TX:    46403
	Packets per second:     100200, Durchsatz:        195 Mbit, RX:   100201, TX:   100201
	Packets per second:     100230, Durchsatz:        195 Mbit, RX:   100204, TX:   100201
	Packets per second:     100234, Durchsatz:        195 Mbit, RX:   100207, TX:   100204
	Packets per second:     100236, Durchsatz:        195 Mbit, RX:   100204, TX:   100201
	Packets per second:     100200, Durchsatz:        195 Mbit, RX:   100202, TX:   100202
	Packets per second:     100183, Durchsatz:        195 Mbit, RX:   100204, TX:   100204
	Packets per second:     100197, Durchsatz:        195 Mbit, RX:   100207, TX:   100201
	Packets per second:     100220, Durchsatz:        195 Mbit, RX:   100204, TX:   100201
	Packets per second:     100200, Durchsatz:        195 Mbit, RX:   100208, TX:   100205
	Packets per second:      51800, Durchsatz:        101 Mbit, RX:    51804, TX:    51801
	Packets per second:          0, Durchsatz:          0 Mbit, RX:        1, TX:        1
	Packets per second:          0, Durchsatz:          0 Mbit, RX:       13, TX:        4
 
"Packets per second" and "Durchsatz" (should be translated to throughput) is what the 
pingpong_bouncer sees, RX and TX are counters from the network card.

On the sender you will get statistics after it stops:


	# Start Session with Packetsize: 256, Threads: 2, Queryrate: 100000
	Laufzeit: 10 s, Dauer Zeitscheibe: 0.001000 s, Zeitscheiben total: 10000, Qpzs: 50, Source: 192.168.155.20:58029
	Laufzeit: 10 s, Dauer Zeitscheibe: 0.001000 s, Zeitscheiben total: 10000, Qpzs: 50, Source: 192.168.155.20:64555
	total idle: 8.609022
	total idle: 8.621772
	Packets send:        1000000, Qps:     100006, Durchsatz:        195 MBit
	Packets received:     999825, Qps:      99988, Durchsatz:        195 MBit
	Packets lost:            175 = 0.018 %
	Errors:                    0, Qps:          0
	Errors 0Byte:              0, Qps:          0
	rtt average: 0.4538 ms
	rtt min:     0.0861 ms
	rtt max:     0.8290 ms






