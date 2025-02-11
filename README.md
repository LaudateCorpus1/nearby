# Nearby C++ Library

The repository contains the Nearby project C++ library source code. This is not an
officially supported Google product.

# About the Nearby Project

## Nearby Connection
Nearby Connections is a high level protocol on top of Bluetooth/WiFi that acts
as a medium-agnostic socket. Devices are able to advertise, scan, and connect
with one another over any shared medium (eg. BT <-> BT).
Once connected, the two devices share a list of all supported mediums and
attempt to upgrade to the one with the highest bandwidth (eg. BT -> WiFi).
The connection is encrypted, reliable, and fully duplex. BYTE, FILE, and STREAM
payloads are all supported and will be chunked & transferred internally and
recombined on the receiving device.
See [Nearby Connections Overview](https://developers.google.com/nearby/connections/overview)
for more information.

# Checkout the source tree

```shell
git clone https://github.com/google/nearby
cd nearby
git submodule update --init --recursive
```

# Building Nearby, Unit Testing and Sample Apps
We support multiple platforms including Linux, iOS & Windows.
## Building for Linux
Currently we support building from source using [bazel] (https://bazel.build). Other BUILD system such as cmake may be added later.

Prerequisites:

1. Bazel
2. clang with support for c++ 17+
3. Openssl libcrypto.so (-lssl;-lcrypto).


To build the Nearby Connection Core library:

```shell
CC=clang CXX=clang++ bazel build -s --check_visibility=false //connections:core  --spawn_strategy=standalone --verbose_failures
```

## iOS
Currently we provide precompiled libraries. See the following page for instructions.

* [iOS](https://github.com/google/nearby/blob/master/docs/ios_build.md)


**Last Updated:** March 2022
