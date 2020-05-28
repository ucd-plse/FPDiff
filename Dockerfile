FROM ubuntu:18.04
WORKDIR /usr/local/src/fp-diff-testing
COPY ./build.sh /usr/local/src/fp-diff-testing/
RUN apt-get update && apt-get install -y \
	gcc \
	g++ \
	make \
	libboost-all-dev \
	python3-pip \
    wget \
    unzip \
    git \
	nodejs \
ENV PYTHONPATH="$PYTHONPATH:/usr/local/lib/python3.6/dist-packages/"
RUN ./build.sh
