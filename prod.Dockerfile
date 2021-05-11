FROM ubuntu:20.04 AS build

# Downloading dependencies
RUN apt-get update && \
    apt-get install -y \
		g++ \
		cmake \
        libavcodec-dev \
        libavformat-dev \
        libswscale-dev \
        libavutil-dev

# Setting up user
RUN useradd -m user && yes password | passwd user

# Configuring working environment
RUN mkdir -p /app
COPY CMakeLists.txt /app
COPY src /app/src

RUN mkdir -p /app/build
WORKDIR /app/build

# Building
RUN cmake -DCMAKE_BUILD_TYPE=Release .. && \
	cmake --build . -j 8

# Running ---------------
FROM ubuntu:20.04 AS run

EXPOSE 8080
EXPOSE 4577/udp

# Downloading dependencies
RUN apt-get update && \
    apt-get install -y \
        libavcodec58 \
        libavformat58 \
        libswscale5 \
        libavutil56

WORKDIR /app
COPY --from=build /app/bin/Release/media-server .
ENTRYPOINT [ "./media-server" ]
