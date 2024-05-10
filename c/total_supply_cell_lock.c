#define CKB_C_STDLIB_PRINTF
#include <stdio.h>

#include "blockchain.h"
#include "ckb_syscalls.h"

// the typeScript specifically designed xudt hard_cap is 293 bytes
#define XUDT_HARDCAP_TYPE_SIZE 293
#define BLAKE2B_BLOCK_SIZE 32
#define XUDT_FLAGS_SIZE 4
#define SCRIPT_VEC_ELEMENT_SIZE 65
#define LOCK_SCRIPT_SIZE 65

// error
#define ERROR_UNLOCK_FAIL 1
#define ERROR_INVALID_MOL_FORMAT 2
#define ERROR_SYSCALL 3
#define ERROR_SCRIPT_TOO_LONG 4
#define ERROR_ENCODING 5

size_t determineTotalSupplyCellOutputIndex() {
  /*
    Load the current script hash - which is a lockScript hash.
    No exception since lockScript is a must at every cell
  */

  uint8_t hash[BLAKE2B_BLOCK_SIZE];
  uint64_t len = BLAKE2B_BLOCK_SIZE;
  int ret = ckb_load_script_hash(hash, &len, 0);

  /*
    Test against lock hash of every cell in the output to determine 
    the index of the cell (that contains typeID total supply)
  */
  size_t index = 0;
  while (1) {
    uint64_t len = BLAKE2B_BLOCK_SIZE;
    uint8_t testHash[BLAKE2B_BLOCK_SIZE];
    ret = ckb_load_cell_by_field(testHash, &len, 0, index, CKB_SOURCE_OUTPUT, CKB_CELL_FIELD_LOCK_HASH);
    if (ret == CKB_ITEM_MISSING) {
      break;
    } else if (ret == CKB_SUCCESS) {
      if (memcmp(hash, testHash, BLAKE2B_BLOCK_SIZE) == 0) {
          /* Found a match */
          return index;
      }
      break;
    } else {
      return CKB_INDEX_OUT_OF_BOUND;
    }
    index++;
  }
  return CKB_INDEX_OUT_OF_BOUND;
}

int main() {
  /*
    Get the total supply cell ouput index and based on the index, 
    fetch the typeScript hash accompanied within the cell
  */
  int ret = 0;
  uint8_t totalSupTypeHash[BLAKE2B_BLOCK_SIZE];
  uint64_t len = BLAKE2B_BLOCK_SIZE;
  size_t findRet = determineTotalSupplyCellOutputIndex();
  if (findRet == CKB_INDEX_OUT_OF_BOUND)
    return ERROR_UNLOCK_FAIL;

  ret = ckb_load_cell_by_field(totalSupTypeHash, &len, 0, findRet, CKB_SOURCE_OUTPUT, CKB_CELL_FIELD_TYPE_HASH);
  if (ret != CKB_SUCCESS)
    return ERROR_UNLOCK_FAIL;

  size_t index = 0;
  while (index < SIZE_MAX) {
      uint64_t len = 32;
      unsigned char typeScript[XUDT_HARDCAP_TYPE_SIZE];
      ret = ckb_load_cell_by_field(typeScript, &len, 0, index, CKB_SOURCE_OUTPUT, CKB_CELL_FIELD_TYPE);
      switch (ret) {
          case CKB_ITEM_MISSING:
              break;
          case CKB_SUCCESS: {
              mol_seg_t typeScriptSeg;
              typeScriptSeg.ptr = typeScript;
              typeScriptSeg.size = len;
              mol_seg_t xudtArgs = MolReader_Script_get_args(&typeScriptSeg);
              // TODO verify if xudtArgs.size == 293
              printf(">>>xudtArgs.size: %d", xudtArgs.size);
              if (xudtArgs.size <= (BLAKE2B_BLOCK_SIZE + XUDT_FLAGS_SIZE)) {
                break;
              } else {
                mol_seg_t serializedScriptVec;
                serializedScriptVec.ptr = xudtArgs.ptr + BLAKE2B_BLOCK_SIZE + XUDT_FLAGS_SIZE;
                serializedScriptVec.size = xudtArgs.size - BLAKE2B_BLOCK_SIZE - XUDT_FLAGS_SIZE;
                uint32_t scriptVecLen = MolReader_ScriptVec_length(&serializedScriptVec);
                // loop over the script vec, process each script
                for (uint8_t i = 0; i < scriptVecLen; i++) {
                  mol_seg_res_t script = MolReader_ScriptVec_get(&serializedScriptVec, i);
                  if(script.errno == 0)
                    break;
                  mol_seg_t args = MolReader_Script_get_args(&script.seg);
                  printf(">>> typeIdHash got from output %d is:", index);
                  for (uint16_t i = 0; i < script.seg.size; i ++) {
                    printf(">>>0x%x", *(script.seg.ptr + i));
                  }
                  // an output with a typeScript that links to the total supply typeId found
                  if (memcmp(args.ptr, totalSupTypeHash, 32) == 0) {
                    return CKB_SUCCESS;
                  }
                }
              }
              break;
          }
          default:
              return ERROR_UNLOCK_FAIL;
      }
      index++;
  }
  return ERROR_UNLOCK_FAIL;
}
