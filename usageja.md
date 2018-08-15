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

### 1. ビルド成功

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
