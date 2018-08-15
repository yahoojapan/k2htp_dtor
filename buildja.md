---
layout: contents
language: en-us
title: Build
short_desc: K2Hash Transaction Plugin Distributed Transaction Of Repeater
lang_opp_file: build.html
lang_opp_word: To English
prev_url: usageja.html
prev_string: Usage
top_url: indexja.html
top_string: TOP
next_url: 
next_string: 
---

# ビルド方法
K2HTPDTORおよびK2HTPDTORSVRをビルドする方法を説明します。

## 1. 事前環境
- Debian / Ubuntu
```
$ sudo aptitude update
$ sudo aptitude install git autoconf autotools-dev gcc g++ make gdb dh-make fakeroot dpkg-dev devscripts libtool pkg-config libssl-dev libyaml-dev
```
- Fedora / CentOS
```
$ sudo yum install git autoconf automake gcc libstdc++-devel gcc-c++ make libtool openssl-devel libyaml-devel
```

## 2. ビルド、インストール：FULLOCK
```
$ git clone https://github.com/yahoojapan/fullock.git
$ cd fullock
$ ./autogen.sh
$ ./configure --prefix=/usr
$ make
$ sudo make install
```

## 3. ビルド、インストール：K2HASH
```
$ git clone https://github.com/yahoojapan/k2hash.git
$ cd k2hash
$ ./autogen.sh
$ ./configure --prefix=/usr
$ make
$ sudo make install
```

## 4. ビルド、インストール：CHMPX
```
$ git clone https://github.com/yahoojapan/chmpx.git
$ cd chmpx
$ ./autogen.sh
$ ./configure --prefix=/usr
$ make
$ sudo make install
```

## 5. clone
```
$ git clone git@github.com:yahoojapan/chmpx.gif
```

## 6. ビルド、インストール：K2HTPDTOR
```
$ ./autogen.sh
$ ./configure --prefix=/usr
$ make
$ sudo make install
```
