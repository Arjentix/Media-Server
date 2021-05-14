# Media Server

This is a server program, that takes video stream from another server, transforms it and returns to clients.

This is my graduate project.

Tested with:

1. My [Raspberry Pi RTSP Server](https://github.com/Arjentix/Pi-RTSP-Server) as a source video server
2. *VLC* 3.0.12 as a HLS client

## Supported features

* Receives **MJPEG**-encoded video by **RTSP/RTP** protocol
* Transcodes it to the **H.264** codec
* Packs this to the **MPEG2-TS** container
* And sends final video to client via **HLS** protocol

## Limitations

First of all, that's a learning project, so there is a few limitations:

1. No audio support
2. No **RTCP** support
3. No config support
4. No file logging support
5. No authorization support
6. No encryption support

All of these limitations can be fixed, but that's not gonna be done within the frameworks of this project.

## Code & Extensibility

> The code of this project is written in the way to allow easy extensibility.

### Media

Every module, that works with media data, is *Observer* and/or *Provider* specified with concrete data type. Observers are subscribed to Providers of the same data.

For example, *rtsp::Client* provides **MJPEG**-encoded frames, so it inherits `Proivder<MjpegFrame>`. In the same time, *MjpegToH264* converter class inherits from `Observer<MjpegFrame>` and `Provider<H264Frame>` and is subscribed to *rtsp::Client*.

So any media-data flow can be easily extended by adding new element in chain. If you want to add class, that would append some text above H264-encoded video-stream and pass it to HLS, you can inherit this class from `Observer<H264Frame>` and `Provider<H264Frame>`, subscribe it to *MjpegToH264* class and subscribe *Mpeg2TsPackager* to it.

### Ports and servlets

You may want to add new protocol to communicate with clients. For example **MPEG-DASH**.

So you might need a specified port for that. All you have to do is creating a new *PortHandler* instance and register it in *PortHandlerManager*. Now your port is watched.

The next thing is servlet. It's a specific class, extended from *Servlet* class. It provides response for given request. So you need to just write such servlet and register it in your *PortHandler* instance on specified path. Now this path is handled by your servlet. You could also register a lot of different servlets on different paths. All request dispatching is already done.

## Dependencies

This project uses **ffmpeg** to transcode and pack video. So to be able to build and run it manually you have to install some packets:

1. On Ubuntu: `sudo apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev `
2. On Arch: `pacman -S ffmpeg`

## Build & Run

### Docker compose

```bash
docker compose -f prod-docker-compose.yml build
docker compose -f prod-docker-compose.yml up
```

### Manual

Make sure you have installed all dependencies 👆

All commands are shown from project root directory.

Build:

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j 8
```

Run:

```bash
bin/Release/media-server <rtsp-stream-url>
```

## Test

To test it you can simple open `http://yourip:8080/playlist.m3u` in *VLC* player