---
layout: contents
language: en-us
title: Overview
short_desc: K2Hash Transaction Plugin Distributed Transaction Of Repeater
lang_opp_file: homeja.html
lang_opp_word: To Japanese
prev_url: 
prev_string: 
top_url: index.html
top_string: TOP
next_url: details.html
next_string: Details
---

# K2HTPDTOR
**K2HTPDTOR** ([**K2H**ash](https://k2hash.antpick.ax/) **T**ransaction **P**lugin **D**istributed **T**ransaction **O**f **R**epeater) is a standard transaction plug-in compatible with the transaction plug-in provided in the [K2HASH](https://k2hash.antpick.ax/) library.  
**K2HTPDTOR** provides general operations for operations on the [K2HASH](https://k2hash.antpick.ax/) file(memory).

**K2HTPDTOR** can transfer the [K2HASH](https://k2hash.antpick.ax/) operation transactions to other hosts using [CHMPX](https://chmpx.antpick.ax/) as the main purpose and easily duplicate the [K2HASH](https://k2hash.antpick.ax/) data.  
As another function, **K2HTPDTOR** accepts transactions and provides a general way to do their own processing.  
By providing this general method, it is possible to perform any processing triggered by a transaction.

## Overview
The following figure shows a model in which **K2HTPDOTR** transfers a transaction that operated [K2HASH](https://k2hash.antpick.ax/) data to another host using [CHMPX](https://chmpx.antpick.ax/) and creates a copy of [K2HASH](https://k2hash.antpick.ax/) data.

![Overview](images/k2htpdtor_overview.png)

The flow of transaction transfer in the above figure is explained.

1. The application program changes the **K2HASH** data.
2. The **K2HASH** library generates the operation of **K2HASH** data as transaction data and hand it over to **K2HTPDTOR**.
3. **K2HTPDTOR** sends transaction data to the **CHMPX** program (server node) via the **CHMPX** program (slave node).
4. The **CHMPX** program (server node) receives the transaction data and passes that data to **K2HTPDTORSVR**.
5. **K2HTPDTORSVR** manipulates transaction data indicated by **K2HASH**, and **K2HASH** can be copied.

In this example, **K2HASH** data is being copied using the **K2HTPDTORSVR** program(this is general receive-only program provided by **K2HTPDTOR**).

The **K2HTPDTOR** program (shared library) can filter transactions and can output archive data of **K2HASH** data from transactions.  
These are set by the configuration of **K2HTPDTOR**.  
The **K2HTPDTORSVR** program can also output archive data of **K2HASH** data from a transaction, activate the specified external programs, deliver the transaction data to them, and re-transfer using **CHMPX**.  
**CHMPX** is used for transfer of transaction data.  
**CHMPX** enables multiplexing, broadcasting, and selective transfer of communications for transfer.
