FROM ubuntu:24.04
RUN export noninteractive; apt update; apt install -y wget libreadline8 libusb-0.1-4 tmux make libmpc3
RUN export noninteractive; apt update; apt install -y gcc libsqlite3-dev

# for compiling prxtool
RUN export noninteractive; apt update; apt install -y automake autoconf g++ git

RUN wget https://github.com/pspdev/pspdev/releases/download/v20250601/pspdev-ubuntu-latest-x86_64.tar.gz -O - | gzip -d | tar -C /usr/local -x
#RUN wget https://github.com/pspdev/pspdev/releases/download/v20240701/pspdev-ubuntu-latest-x86_64.tar.gz -O - | gzip -d | tar -C /usr/local -x
RUN echo 'export PATH="/usr/local/pspdev/bin:$PATH"' > /etc/profile.d/pspsdk.sh
RUN echo 'export LD_LIBRARY_PATH="/usr/local/pspsdk/lib:$LD_LIBRARY_PATH"' >> /etc/profile.d/pspsdk.sh
ENTRYPOINT ["/bin/bash", "-l"]

RUN git clone https://github.com/pspdev/prxtool.git; \
	cd prxtool; \
	./bootstrap; \
	./configure; \
	make; \
	make install

# for raeding header files
RUN export noninteractive; apt update; apt install -y nano less
