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

# ビルド

この章は3つの部分から構成されています。

* ローカル開発用に**K2HTPDTOR**を設定する方法
* ソースコードから**K2HTPDTOR**を構築する方法
* **K2HTPDTOR**のインストール方法

## 1. ビルド環境の構築

**K2HTPDTOR**は主に、[FULLOCK](https://fullock.antpick.ax/indexja.html), [K2HASH](https://k2hash.antpick.ax/indexja.html), [CHMPX](https://chmpx.antpick.ax/indexja.html)に依存します。それぞれの依存ライブラリとヘッダファイルは**K2HTPDTOR**をビルドするために必要です。それぞれの依存ライブラリとヘッダファイルをインストールする方法は2つあります。好きなものを選ぶことができます。

* [GitHub](https://github.com/yahoojapan)から依存ファイルをインストール
  依存ライブラリのソースコードとヘッダファイルをインストールします。あなたはそれぞれの依存ライブラリとヘッダファイルをビルドしてインストールします。
* [packagecloud.io](https://packagecloud.io/antpickax/stable)を使用する
  依存ライブラリのパッケージとヘッダファイルをインストールします。あなたはそれぞれの依存ライブラリとヘッダファイルをインストールするだけです。ライブラリはすでに構築されています。

### 1.1. GitHubから各依存ライブラリとヘッダファイルをインストール

詳細については以下の文書を読んでください。

* [FULLOCK](https://fullock.antpick.ax/buildja.html)
* [K2HASH](https://k2hash.antpick.ax/buildja.html)  
* [CHMPX](https://chmpx.antpick.ax/buildja.html)  

### 1.2. packagecloud.ioから各依存ライブラリとヘッダファイルをインストール

このセクションでは、[packagecloud.io - AntPickax stable repository](https://packagecloud.io/antpickax/stable)から各依存ライブラリとヘッダーファイルをインストールする方法を説明します。

注：前のセクションで各依存ライブラリと[GitHub](https://github.com/yahoojapan)からのヘッダーファイルをインストールした場合は、このセクションを読み飛ばしてください。

最近のDebianベースLinuxの利用者は、以下の手順に従ってください。
```bash
$ sudo apt-get update -y
$ sudo apt-get install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh \
    | sudo bash
$ sudo apt-get install autoconf autotools-dev gcc g++ make gdb libtool pkg-config \
    libyaml-dev libfullock-dev k2hash-dev chmpx-dev -y
$ sudo apt-get install git -y
```

Fedoraの利用者は、以下の手順に従ってください。
```bash
$ sudo dnf makecache
$ sudo dnf install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.rpm.sh \
    | sudo bash
$ sudo dnf install autoconf automake gcc gcc-c++ gdb make libtool pkgconfig \
    libyaml-devel libfullock-devel k2hash-devel chmpx-devel -y
$ sudo dnf install git -y
```

その他最近のRPMベースのLinuxの場合は、以下の手順に従ってください。
```bash
$ sudo yum makecache
$ sudo yum install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.rpm.sh \
    | sudo bash
$ sudo yum install autoconf automake gcc gcc-c++ gdb make libtool pkgconfig \
    libyaml-devel libfullock-devel k2hash-devel chmpx-devel -y
$ sudo yum install git -y
```

## 2. GitHubからソースコードを複製する

[GitHub](https://github.com/yahoojapan)から**K2HTPDTOR**の[ソースコード](https://github.com/yahoojapan/k2htp_dtor)をダウンロードしてください。
```bash
$ git clone https://github.com/yahoojapan/k2htp_dtor.git
```

## 3. ビルドしてインストールする

以下の手順に従って**K2HTPDTOR**をビルドしてインストールしてください。 [GNU Automake](https://www.gnu.org/software/automake/)を使って**K2HTPDTOR**を構築します。
```bash
$ cd k2htp_dtor
$ sh autogen.sh
$ ./configure --prefix=/usr
$ make
$ sudo make install
```

**K2HTPDTOR**のインストールが成功すると、**K2HTPDTOR**のマニュアルページが表示されます。
```bash
$ man k2htpdtor
```
