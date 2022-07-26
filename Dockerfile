FROM fedora:36 as build
WORKDIR /build
# Install base packages
RUN yum update -y && yum install -y tzdata gcc gcc-c++ pcre-devel libpcap-devel bind-utils
# configure timezone to CET
RUN rm -rf /etc/localtime && ln -s /usr/share/zoneinfo/Europe/Berlin /etc/localtime
COPY ./ ./
RUN ./configure && make -j all

# final container build
FROM fedora:36
WORKDIR /tmp
RUN yum update -y && yum install -y tzdata pcre libpcap bind-utils net-tools tcpdump traceroute iproute ethtool
COPY --from=build /build/pingpong_sender /usr/bin
COPY --from=build /build/pingpong_bouncer /usr/sbin
ENV PINGPONG_SERVER="127.0.0.1"
ENV PINGPONG_PORT="9053"
ENV PINGPONG_THREADS="1"
ENV PINGPONG_ANSWER_PACKET_SIZE="0"

#EXPOSE $PINGPONG_PORT
CMD ["/usr/bin/bash", "-c", "/usr/sbin/pingpong_bouncer -s $PINGPONG_SERVER:$PINGPONG_PORT -n $PINGPONG_THREADS -p $PINGPONG_ANSWER_PACKET_SIZE"]
#CMD /bin/sh
