## Abstraction
There's a desire to create hard capped tokens but with [SUDT](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0025-simple-udt/0025-simple-udt.md) it's not possible. Leveraging the modular design of [XUDT](https://talk.nervos.org/t/rfc-extensible-udt/5337), we can build an extension plugin that help create hard-capped tokens.

### 1. Build

`make all-via docker`

### 2. The idea
By this design, the xudt tokens of this type (hard capped) will link to a "remaining amount" cell. In each creation transaction of this type, the "remaining amount cell" will be consumed/destroyed and created with new remaining amount - effectively maintaining a number for "the remaining amount". Guides to create this remaining amount cell will be put below.

### 3. Setting up
#### 3.1. Executable binary cell deployment
- [xudt_rce](https://github.com/nervosnetwork/ckb-production-scripts/blob/master/c/xudt_rce.c) binary cell
- Hard_cap extension binary cell (hard_cap.so)
- "Remaining amount" cell lock script binary (ramt_cell_lock)

*Note*: The "remaining amount cell" must be protected by the lockScript ramt_cell_lock.c

#### 3.2. Composing a remaining amount cell
A remaining amount cell, just like any other cell, contains following major fields:
```json
{
    lock: <hard-cap-lock-script>,
    type: <typeid>,
    data: <remaining-amount::4bytes>
}
```
- With hard-cap-lock-script:
```json
{
    codeHash: <remaining-amount-cell-binary-hash>,
    hashType: "data1",
    args: "0x"
}
```
- With typeId: to create a typeId, follow [this guide](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0022-transaction-structure/0022-transaction-structure.md#type-id).
- With remaining-amount: a 4 bit LE hexa number that holds the value of the remaining amount.

#### 3.3. Composing our hard-capped token's cell
A hard-capped XUDT token cell, just like any other cell, contains following major fields:

```json
{
    lock: <owner-lock>,
    type: <hard-capped-xudt-type-script>,
    data: <amount>+ ...
}
```

The important part is the hard-capped-xudt-type-script which lables + mark our tokens:
```json
codeHash: <xudt_rce code hash>
hashType: "data1"
args: <owner's lockscript hash> + <xudt flag> + <ScriptVector>
```

- With Script vector contains 1 Script element:
```json
codeHash: <hard_cap code hash>
hashType: "data1"
args: <remaining amount cell's typeId hash>
```

### 4. Creating tokens
In three modes: creation, transfer, and burn. This extension only has effects on the 'creation' mode.

Token creation is as simple as with SUDT but it is required to place the "remaining amount" cell in the inputs, and calculate another "remaning supply" in the outputs and inputs with the correct remaining-amount calculation. Otherwise token creation will fail

<img width="462" alt="Screenshot 2024-05-08 at 15 05 10" src="https://github.com/tea2x/ckb-xudt-extension-plugins/assets/70423834/16a2d9aa-ddfb-485a-975a-55866f49bf98">

### 5. Note
When you see `hashType: "data1",` present in a lockscript or a typescript in this doc, it means the lock or type are not upgradeable. This repo is a personal project and it might contain bugs and need some auditing work to be deploy on-chain forever with option "data1". Consider to use `hashType: "type",` and make it upgradeable when needed. For upgradeable scripts, follow https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0022-transaction-structure/0022-transaction-structure.md#upgradable-script for more information. 

### 6. Example code in lumos
https://github.com/tea2x/developer-training-course/blob/master/Lab-XUDT/index.js
