---
layout: contents
language: en-us
title: Details
short_desc: K2Hash Transaction Plugin Distributed Transaction Of Repeater
lang_opp_file: detailsja.html
lang_opp_word: To Japanese
prev_url: home.html
prev_string: Overview
top_url: index.html
top_string: TOP
next_url: usage.html
next_string: Usage
---

# Details
## K2HTPDTOR Configuration
K2HTPDTOR is invoked to perform transaction processing from the [K2HASH](https://k2hash.antpick.ax/) library.

First, the configuration of the K2HTPDTOR program (shared library) is passed through the K2HASH library when K2HASH is initialized.  
When the program using the K2HASH library calls the I/F of the following K2HASH library, the configuration is handed over to K2HTPDTOR.  
For example.
```
k2h_enable_transaction_param(handle, NULL, NULL, 0, (const unsigned char*)("/etc/k2ash/mydtor_slave.ini"), 28);
```

K2HTPDTOR is initialized according to the contents of the configuration.  
Configuration is a configuration file (INI format, YAML format, JSON format) or JSON string.  
The items that can be set in the configuration are shown below.

### K2HTPDTOR Section(\[K2HTPDTOR\] is used for INI file)
#### K2HTPDTOR_BROADCAST
Specify whether to allow broadcast transfer (yes / no). It is optional, default is not to broadcast.
#### K2HTPDTOR_CHMPXCONF
When transferring a transaction, specify the configuration of the [CHMPX](https://chmpx.antpick.ax/) slave node.  
It is specified with a configuration file(formatted by INI, YAML, JSON) or storing of JSON.
#### K2HTPDTOR_EXCEPT_KEY
K2HTPDOTR can filter transaction data before processing.  
This setting is to filter by using the key name that is the target of transaction data (change of K2HASH data).  
The key name is expressed as a prefix.
#### K2HTPDTOR_FILE
To output the received transaction data as an archive file of K2HASH, specify the file path.
#### K2HTPDTOR_FILTER_TYPE
In the type of transaction data received, specify the type to filter.  
Multiple types can be specified for this item.  
If this item is not specified, filtering by type will not be done.  
Specify one or more of the following types. (To specify more than one, separate them with ",")
- SETALL  
Transaction which is updated for all(keys, subkeys, attributes)
- DELKEY  
Removing key transaction
- OWVAL  
Transaction of Over writing value to key
- REPVAL  
Replacing value transaction
- REPSKEY  
Replacing subkey transaction
- REPATTR  
Replacing attributes transaction
- RENAME  
Renaming key name transaction

## K2HTPDTORSVR
### Usage K2HTPDTORSVR
To start the K2HTPDTORSVR program, do as follows.
```
k2htpdtorsvr [-conf <path> | -json <string>] [-d <debug level>] [-dlog <file path>]
```

### K2HTPDTORSVR Options
The options of the K2HTPDTORSVR program are summarized below.
#### -h
display help for the options of K2HTPDTORSVR program
#### -ver
display version of K2HTPDTORSVR program
#### -conf
Specify the configuration file(formatted by INI, YAML, JSON) for CHMPX server node. This option is exclusive with the -json option.
#### -json
Specify the configuration by JSON string for CHMPX server node. This option is exclusive with the -conf option.
#### -d \<debug level\>
Specify the level of output message. The level value is silent, err, wan, info or dump. as default silent.
#### -dlog \<file path\>
Specify the file path which the output message puts.

### Environments for K2HTPDTORSVR
#### DTORSVRCONFFILE
Specify the configuration file(formatted by INI, YAML, JSON) for CHMPX server node.
#### DTORSVRJSONCONF
Specify the configuration by JSON string for CHMPX server node.

_If you do not specify both -conf and -json option, K2HTPDTORSVR checks DTORSVRCONFFILE or DTORSVRJSONCONF environments._  
_If there is not any option and environment for configuration, you can not run CHMPX program with error._

### K2HTPDTORSVR Configuration
#### K2HTPDTORSVR Section(\[K2HTPDTORSVR\] is used for INI file)
- OUTPUTFILE  
When outputting the archive file of K2HASH, specify the file path. If not specified, the K2HASH archive file is not output.
- PLUGIN  
When executing an external program, specify the file path. If not specified, the program will not be executed.  
For the value, you can specify the environment variable, program path, and argument to be passed to the external program.  
```
For example: PLUGIN = myenv = on yourenv = off / bin / myprogram myargs 1
```
- K2HTYPE  
When outputting the transaction to the K2HASH file(memory), specify the type of K2HASH.  
Possible values are mem(memory type), file(file type), temp(temporary file type).
- K2HFILE  
If K2HTYPE is file/temp, specify the file path. (In the case of temp, you can omit it)
- K2HFULLMAP  
When K2HTYPE is file/temp, it specifies whether to map all K2HASH files to memory or not. (In the case of mem, all items are mapped, so this item is ignored.)
- K2HINIT  
When K2HTYPE is file, specify whether or not to initialize the K2HASH file. (In the case of mem/temp, it is always initialized and this item is ignored.)
- K2HMASKBIT  
Specify the setting value of K2HASH. As default 8. (See [K2HASH](https://k2hash.antpick.ax/) )
- K2HCMASKBIT  
Specify the setting value of K2HASH. As default 4. (See [K2HASH](https://k2hash.antpick.ax/) )
- K2HMAXELE  
Specify the setting value of K2HASH. As default 8. (See [K2HASH](https://k2hash.antpick.ax/) )
- K2HPAGESIZE  
Specify the setting value of K2HASH. As default 128. (See [K2HASH](https://k2hash.antpick.ax/) )
- K2HDTORSVR_TRANS  
K2HTPDTORSVR can output transaction data to the K2HASH file and retransmit the transaction data to another server.  
In this item, you specify the configuration of the K2HTPDTOR program to be used for retransferred.  
If this item is omitted, it means that this program is the end of transaction transfer.
- DTORTHREADCNT  
When retransmitting transaction data to another server, specify the number of threads of the K2HTPDTOR program. It is 1 if omitted. (Please refer to the explanation of [K2HASH](https://k2hash.antpick.ax/) )

### Execution of external program
When executing an external program, the external program is started as a child process of K2HTPDTORSVR.  
Transaction data is written to the standard input of the external program.
