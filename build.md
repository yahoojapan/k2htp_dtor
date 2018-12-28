---
layout: contents
language: en-us
title: Build
short_desc: K2Hash Transaction Plugin Distributed Transaction Of Repeater
lang_opp_file: buildja.html
lang_opp_word: To Japanese
prev_url: usage.html
prev_string: Usage
top_url: index.html
top_string: TOP
next_url: 
next_string: 
---

# Build

This chapter consists of three parts:

* how to set up **K2HTPDTOR** for local development
* how to build **K2HTPDTOR** from the source code
* how to install **K2HTPDTOR**.

## 1. Install prerequisites

**K2HTPDTOR** primarily depends on [FULLOCK](https://fullock.antpick.ax/index.html), [K2HASH](https://k2hash.antpick.ax/index.html), [CHMPX](https://chmpx.antpick.ax/index.html). Each dependent library and the header files are required to build **K2HTPDTOR**. We provide two ways to install them. You can select your favorite one.

* Use [GitHub](https://github.com/yahoojapan)  
  Install the source code of dependent libraries and the header files. You will **build** them and install them.
* Use [packagecloud.io](https://packagecloud.io/)  
  Install packages of dependent libraries and the header files. You just install them. Libraries are already built.

### 1.1. Install each dependent library and the header files from GitHub

Read the following documents for details:

* [fullock](https://fullock.antpick.ax/build.html)
* [k2hash](https://k2hash.antpick.ax/build.html)
* [chmpx](https://chmpx.antpick.ax/build.html)

### 1.2. Install each dependent library and the header files from packagecloud.io

This section instructs how to install each dependent library and the header files from [packagecloud.io - AntPickax stable repository](https://packagecloud.io/antpickax/stable). 

Note: Skip reading this section if you have installed each dependent library and the header files from [GitHub](https://github.com/yahoojapan) in the previous section.

For DebianStretch or Ubuntu(Bionic Beaver) users, follow the steps below:
```bash
$ sudo apt-get update -y
$ sudo apt-get install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh \
    | sudo bash
$ sudo apt-get install autoconf autotools-dev gcc g++ make gdb libtool pkg-config \
    libyaml-dev libfullock-dev k2hash-dev chmpx-dev -y
$ sudo apt-get install git -y
```

For Fedora28 or CentOS7.x(6.x) users, follow the steps below:
```bash
$ sudo yum makecache
$ sudo yum install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.rpm.sh \
    | sudo bash
$ sudo yum install autoconf automake gcc gcc-c++ gdb make libtool pkgconfig \
    libyaml-devel libfullock-devel k2hash-devel chmpx-devel -y
$ sudo yum install git -y
```

## 2. Clone the source code from GitHub

Download the **K2HTPDTOR**'s source code from [GitHub](https://github.com/yahoojapan/k2htp_dtor).
```bash
$ git clone https://github.com/yahoojapan/k2htp_dtor.git
```

## 3. Build and install

Just follow the steps below to build **K2HTPDTOR** and install it. We use [GNU Automake](https://www.gnu.org/software/automake/) to build **K2HTPDTOR**.
```bash
$ cd k2htp_dtor
$ sh autogen.sh
$ ./configure --prefix=/usr
$ make
$ sudo make install
```

After successfully installing **K2HTPDTOR**, you will see the manual page for **K2HTPDTOR**:
```bash
$ man k2htpdtor
```
