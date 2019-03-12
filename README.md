# UDPPingPong
Tools for measuring udp throughput:
- pingpong_sender: generates packets of arbitrary size and amount and counts
  the answers and throughput
- pingpong_bouncer: recieves the packets on the other side and sends them back.


# Build and install
./configure
make
make install

By default the binaries will be installed in /usr/local/bin. If you want them
somewhere else (e.g. /usr/bin), please use ./configure --prefix:

./configure --prefix=/usr

# Uninstall
make uninstall






