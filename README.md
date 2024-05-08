## Abstraction
There's a desire to create hard capped tokens but with sudt it's not possible. Leveraging the modular design of XUDT, we can build extension plugins that help create limited tokens.

hard_cap.c is A Dynamic Link Library script-extensions that work with CKB XUDT standard https://github.com/nervosnetwork/ckb-production-scripts/blob/master/c/xudt_rce.c, allowing for the creation of limited tokens.

### 1. Build

`make all-via docker`

### 2. How to use this plugin

#### 2.1. Cell deployment
    - [xudt_rce](https://github.com/nervosnetwork/ckb-production-scripts/blob/master/c/xudt_rce.c) binary cell
    - Hard_cap extension binary cell
    - Total supply(remaining) cell: This is a [typeId cell](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0022-transaction-structure/0022-transaction-structure.md#type-id) with total supply info placed in the data field of the cell. TypeID ensure the singleton of totalsupply cell.

<img width="462" alt="Screenshot 2024-05-08 at 15 03 40" src="https://github.com/tea2x/ckb-xudt-extension-plugins/assets/70423834/36be678d-f42a-44ea-8258-64d273ae955e">

*Note*: For transparence and security reasons, it is recommended to deploy these three cells as ownerless cells - meaning no one owns it. As so, total supply or xudt/extension logic will not be tampered with.

#### 2.2. Composing our hard-capped token's typeScript
    The typeScript for an XUDT with hard_cap extension by this design is:
```json
codeHash: <xudt_rce code hash>
hashType: "data1"
args: <owner's lockscript hash> + <xudt flag> + <ScriptVector*>
```

(*) Script vector contains 1 Script element:
```json
codeHash: <hard_cap code hash>
hashType: "data1"
args: <remaining cell's typeId hash>
```

#### 2.3 create tokens
In three modes: creation, transfer, and burn. This extension only has effects on the 'creation' mode.

Token creation is as simple as with SUDT but it is required to place the Total supply cell in the inputs, and calculate another "remaning supply" in the outputs. Otherwise token creation will fail

<img width="462" alt="Screenshot 2024-05-08 at 15 05 10" src="https://github.com/tea2x/ckb-xudt-extension-plugins/assets/70423834/16a2d9aa-ddfb-485a-975a-55866f49bf98">
