---
layout: contents
language: ja
title: Usage
short_desc: K2Hash Transaction Plugin Distributed Transaction Of Repeater
lang_opp_file: usage.html
lang_opp_word: To English
prev_url: detailsja.html
prev_string: Details
top_url: indexja.html
top_string: TOP
next_url: buildja.html
next_string: Build
---

# 使い方

## サンプルコンフィグレーション
K2HTPDTORとK2HTPDTORSVRの利用するコンフィグレーションのサンプルを示します。

### 再転送された終端サーバー上のコンフィグレーション
#### K2HTPDTORSVR
- INI形式  
[dtor_test_trans_server.ini]({{ site.github.repository_url }}/blob/master/tests/dtor_test_trans_server.ini)
- YAML形式  
[dtor_test_trans_server.yaml]({{ site.github.repository_url }}/blob/master/tests/dtor_test_trans_server.yaml)
- JSON形式  
[dtor_test_trans_server.json]({{ site.github.repository_url }}/blob/master/tests/dtor_test_trans_server.json)
- JSON文字列  
[test_json_string.data]({{ site.github.repository_url }}/blob/master/tests/test_json_string.data) ファイルの中の "DTOR_TEST_TRANS_SERVER_JSON=" の以降の文字列

#### CHMPX
K2HTPDTORSVRと同じコンフィグレーションです。

### 再転送する中継サーバー上のコンフィグレーション
#### K2HTPDTORSVR
- INI形式  
[dtor_test_server.ini]({{ site.github.repository_url }}/blob/master/tests/dtor_test_server.ini)
- YAML形式  
[dtor_test_server.yaml]({{ site.github.repository_url }}/blob/master/tests/dtor_test_server.yaml)
- JSON形式  
[dtor_test_server.json]({{ site.github.repository_url }}/blob/master/tests/dtor_test_server.json)
- JSON文字列  
[test_json_string.data]({{ site.github.repository_url }}/blob/master/tests/test_json_string.data) ファイルの中の "DTOR_TEST_SERVER_JSON=" の以降の文字列

#### CHMPX（スレーブノード：再転送先への接続）
- INI形式  
[dtor_test_trans_slave.ini]({{ site.github.repository_url }}/blob/master/tests/dtor_test_trans_slave.ini)
- YAML形式  
[dtor_test_trans_slave.yaml]({{ site.github.repository_url }}/blob/master/tests/dtor_test_trans_slave.yaml)
- JSON形式  
[dtor_test_trans_slave.json]({{ site.github.repository_url }}/blob/master/tests/dtor_test_trans_slave.json)
- JSON文字列  
[test_json_string.data]({{ site.github.repository_url }}/blob/master/tests/test_json_string.data) ファイルの中の "DTOR_TEST_TRANS_SLAVE_JSON=" の以降の文字列

#### CHMPX（サーバーノード：転送元からの接続）
K2HTPDTORSVRと同じコンフィグレーションです。

### 転送元のK2HTPDTORが使用するコンフィグレーション
#### K2HTPDTOR
- INI形式  
[dtor_test_slave.ini]({{ site.github.repository_url }}/blob/master/tests/dtor_test_slave.ini)
- YAML形式  
[dtor_test_slave.yaml]({{ site.github.repository_url }}/blob/master/tests/dtor_test_slave.yaml)
- JSON形式  
[dtor_test_slave.json]({{ site.github.repository_url }}/blob/master/tests/dtor_test_slave.json)
- JSON文字列  
[test_json_string.data]({{ site.github.repository_url }}/blob/master/tests/test_json_string.data) ファイルの中の "DTOR_TEST_SLAVE_JSON=" の以降の文字列

## 簡単な動作確認
K2HTPDTORおよびK2HTPDTORSVRをビルドした後で、簡単な動作確認をします。  
動作確認は、トランザクションを再転送する動作を確認します。

### 1. 利用環境構築

**K2HTPDTOR** をご利用の環境にインストールするには、2つの方法があります。  
ひとつは、[packagecloud.io](https://packagecloud.io/)から **K2HTPDTOR** のパッケージをダウンロードし、インストールする方法です。  
もうひとつは、ご自身で **K2HTPDTOR** をソースコードからビルドし、インストールする方法です。  
これらの方法について、以下に説明します。

#### パッケージを使ったインストール
**K2HTPDTOR** は、誰でも利用できるように[packagecloud.io - AntPickax stable repository](https://packagecloud.io/antpickax/stable/)で[パッケージ](https://packagecloud.io/app/antpickax/stable/search?q=k2htpdtor)を公開しています。  
**K2HTPDTOR** のパッケージは、Debianパッケージ、RPMパッケージの形式で公開しています。  
お使いのOSによりインストール方法が異なりますので、以下の手順を確認してインストールしてください。  

##### Debian(Stretch) / Ubuntu(Bionic Beaver)
```
$ sudo apt-get update -y
$ sudo apt-get install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.deb.sh | sudo bash
$ sudo apt-get install k2htpdtor chmpx
```

##### Fedora28 / CentOS7.x(6.x)
```
$ sudo yum makecache
$ sudo yum install curl -y
$ curl -s https://packagecloud.io/install/repositories/antpickax/stable/script.rpm.sh | sudo bash
$ sudo yum install k2htpdtor
```

##### 上記以外のOS
上述したOS以外をお使いの場合は、パッケージが準備されていないため、直接インストールすることはできません。  
この場合には、後述の[ソースコード](https://github.com/yahoojapan/k2htp_dtor)からビルドし、インストールするようにしてください。

#### ソースコードからビルド・インストール
**K2HTPDTOR** を[ソースコード](https://github.com/yahoojapan/k2htp_dtor)からビルドし、インストールする方法は、[ビルド](https://k2htpdtor.antpick.ax/buildja.html)を参照してください。

### 2. 終端ホストのCHMPXサーバーノードを起動
```
$ chmpx -conf dtor_test_trans_server.ini
```

### 3. 終端ホストのK2HTPDTORSVRを起動
```
$ k2htpdtorsvr -conf dtor_test_trans_server.ini
```

### 4. 中継ホスト上の終端CHMPXと接続するCHMPX（スレーブノード）を起動
```
$ chmpx -conf dtor_test_trans_slave.ini
```

### 5. 中継ホスト上の転送元が接続するCHMPX（サーバーノード）を起動
```
$ chmpx -conf dtor_test_server.ini
```

### 6. 中継ホストのK2HTPDTORSVRを起動
```
$ k2htpdtorsvr -conf dtor_test_server.ini
```

### 7. 転送元ホスト上の中継CHMPXと接続するCHMPX（スレーブノード）を起動
```
$ chmpx -conf dtor_test_slave.ini
```

### 8. 転送元ホスト上のトランザクションを発生させるテストプログラムを起動
```
$ k2htpdtorclient -f dtor_test_archive.data -l k2htpdtor.so -p dtor_test_slave.ini -c 1
```
_このプログラムはテスト専用プログラムです。_

以上のようにして、トランザクションが転送できていることが確認できます。
各プログラムは、シグナルHUPを送ることにより終了できます。
