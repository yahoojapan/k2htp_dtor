---
layout: contents
language: ja
title: Details
short_desc: K2Hash Transaction Plugin Distributed Transaction Of Repeater
lang_opp_file: details.html
lang_opp_word: To English
prev_url: homeja.html
prev_string: Overview
top_url: indexja.html
top_string: TOP
next_url: usageja.html
next_string: Usage
---

# 詳細説明 
## K2HTPDTORコンフィグレーション
K2HTPDTORは [K2HASH](https://k2hash.antpick.ax/indexja.html) ライブラリからトランザクション処理を行うために呼び出されます。  
まず、K2HTPDTORプログラム（シェアードライブラリ）は、初期化のタイミングでコンフィグレーションをK2HASHライブラリを通して渡されます。

K2HASHライブラリを利用するプログラムから以下のAPI I/Fを通してコンフィグレーションが引き渡されます。  
以下は例です。
```
k2h_enable_transaction_param(handle, NULL, NULL, 0, (const unsigned char*)("/etc/k2ash/mydtor_slave.ini"), 28);
```

このコンフィグレーションにより、K2HTPDTORの設定がなされます。  
コンフィグレーションは、コンフィグレーションファイル（INI形式、YAML形式、JSON形式）もしくはJSON文字列で記述されます。  
コンフィグレーション設定できる内容を以下に示します。

### K2HTPDTOR（INIファイル形式時は"[K2HTPDTOR]"）セクション
#### K2HTPDTOR_BROADCAST
ブロードキャスト可否（yes/no）を指定します。省略可能であり、デフォルトはブロードキャストをしない設定です。
#### K2HTPDTOR_CHMPXCONF
トランザクションを転送する場合の [CHMPX](https://chmpx.antpick.ax/indexja.html) スレーブノードのコンフィグレーションを指定します。  
コンフィグレーションファイル（INI形式、YAML形式、JSON形式）もしくは、JSON文字列で指定します。
#### K2HTPDTOR_EXCEPT_KEY
受け取ったトランザクションデータの中にあるK2HASHに書き込まれたキー名のプレフィックスを指定して、フィルタリングできます。  
フィルタリングしたいキーのプレフィックスを指定します。
#### K2HTPDTOR_FILE
トランザクションデータをK2HASHのアーカイブファイルとして出力する場合に、ファイルパスを指定します。
#### K2HTPDTOR_FILTER_TYPE
受け取ったトランザクションデータのタイプで、フィルタリングするタイプを指定します。  
複数のタイプを指定することが可能です。本項目が指定されていない場合にはフィルタリングはされません。  
以下の1つ以上のタイプを指定します。（複数指定する場合には、","で区切ってください）
- SETALL  
キー、サブキー、属性を全てを書き込んだトランザクション
- DELKEY  
キーの削除のトランザクション
- OWVAL  
オーバーライトのトランザクション
- REPVAL  
値の入れ替えのトランザクション
- REPSKEY  
サブキーの入れ替えのトランザクション
- REPATTR  
属性の入れ替えのトランザクション
- RENAME  
キー名の変更のトランザクション

## K2HTPDTORSVR起動オプション
K2HTPDTORSVR起動は、以下のように行います。
```
k2htpdtorsvr [-conf <path> | -json <string>] [-d <debug level>] [-dlog <file path>]
```

K2HTPDTORSVRの起動オプションを説明します。
### -h
ヘルプを表示します。
### -ver
バージョンを表示します。
### -conf
サーバーノードのCHMPXのコンフィグレーションを含むコンフィグレーションファイル（INI形式、YAML形式、JSON形式）を指定します。このパラメータは-jsonと排他です。
### -json
サーバーノードのCHMPXのコンフィグレーションを含むJSON文字列のコンフィグレーションを指定します。このパラメータは-confと排他です。
### -d <debug level>
メッセージ出力のモードを指定します。SILENT/ERR/WARN/INFOが指定できます。デフォルトはSILENTです。
### -dlog <file path>
メッセージ出力を指定したファイルに出力する場合にファイルパスを指定します。指定されていない場合は、標準エラーに出力されます。

## K2HTPDTORSVRの環境変数
### DTORSVRCONFFILE
サーバーノードのCHMPXのコンフィグレーションを含むコンフィグレーションファイル（INI形式、YAML形式、JSON形式）を指定します。
### DTORSVRJSONCONF
サーバーノードのCHMPXのコンフィグレーションを含むJSON文字列のコンフィグレーションを指定します。

_起動オプション-conf/-jsonもしくは環境変数 DTORSVRCONFFILE/DTORSVRJSONCONF のいずれかでコンフィグレーションを指定してください。_

## K2HTPDTORSVRコンフィグレーション

### K2HTPDTORSVR（INIファイル形式時は"[K2HTPDTORSVR]"）セクション
#### OUTPUTFILE
K2HASHのアーカイブファイルを出力する場合にファイルパスを指定します。省略した場合には、K2HASHアーカイブファイルの出力はなされません。
#### PLUGIN
任意の外部プログラムを実行する場合に、そのファイルパスを指定します。省略した場合には、プログラム実行はされません。  
このキーワードには以下に示すように、環境変数、プログラムパス、引数を指定できます。
```
例： PLUGIN = myenv=on yourenv=off /bin/myprogram myargs1
```
- K2HTYPE  
トランザクションをK2HASHファイルへ出力する場合に、そのK2HASHファイルのタイプを指定します。  
指定できる値は、mem（メモリモード）、file（ファイルモード）、temp（テンポラリファイルモード）の3タイプです。
- K2HFILE  
K2HTYPEがfile/tempの場合に、ファイルパスを指定します。（tempの場合には省略できます）
- K2HFULLMAP  
K2HTYPEがfile/tempの場合に、K2HASHファイルをフルマッピングするか否かを指定します。（memの場合にはフルマッピングであり、本項目を指定する必要はありません）
- K2HINIT  
K2HTYPEがfileの場合に、K2HASHファイルを初期化するか否かを指定します。（mem/tempの場合は必ず初期化されまので、本項目を指定する必要はありません）
- K2HMASKBIT  
K2HASHファイルのmaskbitを指定します。（[K2HASH](https://k2hash.antpick.ax/indexja.html)の説明を参照してください）
- K2HCMASKBIT  
K2HASHファイルのcmaskbitを指定します。（[K2HASH](https://k2hash.antpick.ax/indexja.html)の説明を参照してください）
- K2HMAXELE  
K2HASHファイルのelement countを指定します。（[K2HASH](https://k2hash.antpick.ax/indexja.html)の説明を参照してください）
- K2HPAGESIZE  
K2HASHファイルのpagesizeを指定します。（[K2HASH](https://k2hash.antpick.ax/indexja.html)の説明を参照してください）
- K2HDTORSVR_TRANS  
トランザクションデータをK2HASHファイルへ出力し、かつそのトランザクションデータを他サーバーに再転送する場合に、転送するためのk2htpdtorのコンフィグレーションを指定します。  
本項目が省略された場合には、本プログラムがトランザクション転送の終端となることを意味します。
- DTORTHREADCNT  
トランザクションデータを他サーバーに再転送する場合に、トランザクションプラグインk2htpdtorを起動するスレッド数を指定します。省略された場合には1となります。（[K2HASH](https://k2hash.antpick.ax/indexja.html)の説明を参照してください）

### 外部プログラムの実行
外部プログラムの実行を行う場合、外部プログラムはK2HTPDTORSVR起動時に子プロセスとして起動されます。  
起動後は、トランザクションのデータが外部プログラムの標準入力に書き込まれます。
