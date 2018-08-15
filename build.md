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

# Building
The build method for K2HTPDTOR and K2HTPDTORSVR is explained below.

## 1. Install prerequisites before compiling
- Debian / Ubuntu
```
$ sudo aptitude update
$ sudo aptitude install git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config libssl-dev libyaml-dev
```
- Fedora / CentOS
```
$ sudo yum install git autoconf automake gcc libstdc++-devel gcc-c++ make libtool openssl-devel libyaml-devel
```

## 2. Building and installing FULLOCK
```
$ git clone https://github.com/yahoojapan/fullock.git
$ cd fullock
$ ./autoge,sh
$ ./configure --prefix=/usr
$ make
$ sudo make install
```

## 3. Building and installing K2HASH
```
$ git clone https://github.com/yahoojapan/k2hash.git
$ cd k2hash
$ ./autoge,sh
$ ./configure --prefix=/usr
$ make
$ sudo make install
```

## 4. Building and installing CHMPX
```
$ git clone https://github.com/yahoojapan/chmpx.git
$ cd chmpx
$ ./autoge,sh
$ ./configure --prefix=/usr
$ make
$ sudo make install
```

## 5. Clone source codes from Github
```
$ git clone git@github.com:yahoojapan/chmpx.gif
```

## 6. Building and installing K2HTPDTOR/K2HTPDTORSVR
```
$ ./autoge,sh
$ ./configure --prefix=/usr
$ make
$ sudo make install
```
