FROM ubuntu:20.04 AS build

# Downloading dependencies
RUN apt-get update && \
    apt-get install -y \
		g++ \
		cmake

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

EXPOSE 2020

WORKDIR /app
COPY --from=build /app/bin/media-server .
ENTRYPOINT [ "./media-server" ]
