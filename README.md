## Abstraction
There's a desire to create hard capped tokens but with sudt it's not possible. Leveraging the modular design of XUDT, we can build extension plugins that help create hard-capped tokens.

hard_cap.c is A Dynamic Link Library script-extension that works with [CKB XUDT standard](https://github.com/nervosnetwork/ckb-production-scripts/blob/master/c/xudt_rce.c), allowing for the creation of hard-capped tokens.

### 1. Build

`make all-via docker`

### 2. How to use this plugin
By this design, each xudt token of this type (hard capped) will link to a "total supply" cell. This "total supply cell" will be accompanying our xutd creations and record the change to the remaning mintable tokens. Guides to create this total supply cell will be put below at 2.2

#### 2.1. Cell deployment
- [xudt_rce](https://github.com/nervosnetwork/ckb-production-scripts/blob/master/c/xudt_rce.c) binary cell
- Hard_cap extension binary cell (hard_cap.c)
- Total supply cell lock script binary (total_supply_cell_lock.c)

<img width="462" alt="Screenshot 2024-05-08 at 15 07 29" src="https://github.com/tea2x/ckb-xudt-extension-plugins/assets/70423834/5c037c49-ad67-48d3-b92c-9c7062e81b58">

*Note*: For transparence and security reasons, it is recommended to deploy extension script cell as an ownerless cells - meaning no one owns it. As so, total supply cell must be protected by lockScript total_supply_cell_lock.c

#### 2.2. Create a total supply cell
- lock: hard cap lock script
- type: typeId
- data: <total_supply::4bytes>

Note: to create a typeId, follow [typeId](https://github.com/nervosnetwork/rfcs/blob/master/rfcs/0022-transaction-structure/0022-transaction-structure.md#type-id).

#### 2.3. Composing our hard-capped token's typeScript
The typeScript for an XUDT with hard_cap extension by this design is:
```
codeHash: <xudt_rce code hash>
hashType: "data1"
args: <owner's lockscript hash> + <xudt flag> + <ScriptVector*>
```

(*) Script vector contains 1 Script element:
```json
codeHash: <hard_cap code hash>
hashType: "data1"
args: <total supply cell's typeId hash>
```

#### 2.3 Creating tokens
In three modes: creation, transfer, and burn. This extension only has effects on the 'creation' mode.

Token creation is as simple as with SUDT but it is required to place the Total supply cell in the inputs, and calculate another "remaning supply" in the outputs. Otherwise token creation will fail

<img width="462" alt="Screenshot 2024-05-08 at 15 05 10" src="https://github.com/tea2x/ckb-xudt-extension-plugins/assets/70423834/16a2d9aa-ddfb-485a-975a-55866f49bf98">
