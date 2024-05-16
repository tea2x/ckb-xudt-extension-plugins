// #define CKB_C_STDLIB_PRINTF
// #include <stdio.h>

#include "blockchain.h"
#include "ckb_syscalls.h"

// the typeScript specifically designed xudt hard_cap is 162 bytes
#define XUDT_HARDCAP_TYPE_SIZE 32768
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

size_t determine_rmng_amt_cell_output_index() {
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
    uint8_t test_hash[BLAKE2B_BLOCK_SIZE];
    ret = ckb_load_cell_by_field(test_hash, &len, 0, index, CKB_SOURCE_OUTPUT, CKB_CELL_FIELD_LOCK_HASH);
    switch(ret) {
      case CKB_ITEM_MISSING:
        break;
      case CKB_SUCCESS:
        if (memcmp(hash, test_hash, BLAKE2B_BLOCK_SIZE) == 0) {
          /* Found a match */
          return index;
        }
        break;
      default:
        return CKB_INDEX_OUT_OF_BOUND;
    }
    index++;
  }
  return CKB_INDEX_OUT_OF_BOUND;
}

// check if the type script links to the total-supply cell
bool check_link(unsigned char * type_script, unsigned long long len, unsigned char * rmng_amt_type_hash) {
  mol_seg_t type_script_seg;
  type_script_seg.ptr = type_script;
  type_script_seg.size = len;
  mol_seg_t xudt_args = MolReader_Script_get_args(&type_script_seg);
  mol_seg_t args_bytes_seg = MolReader_Bytes_raw_bytes(&xudt_args);
  if (xudt_args.size > (BLAKE2B_BLOCK_SIZE + XUDT_FLAGS_SIZE)) {
    mol_seg_t serialized_script_vec;
    serialized_script_vec.ptr = args_bytes_seg.ptr + BLAKE2B_BLOCK_SIZE + XUDT_FLAGS_SIZE;
    serialized_script_vec.size = args_bytes_seg.size - BLAKE2B_BLOCK_SIZE - XUDT_FLAGS_SIZE;
    uint32_t script_vec_len = MolReader_ScriptVec_length(&serialized_script_vec);
    // loop over the script vec, process each script
    for (uint8_t i = 0; i < script_vec_len; i++) {
      mol_seg_res_t script = MolReader_ScriptVec_get(&serialized_script_vec, i);
      if(script.errno != 0) {
        break;
      }
      mol_seg_t args = MolReader_Script_get_args(&script.seg);
      mol_seg_t args_bytes_seg = MolReader_Bytes_raw_bytes(&args);
      
      // an output with a typeScript that links to the total supply typeId found
      if (memcmp(args_bytes_seg.ptr, rmng_amt_type_hash, 32) == 0) {
        return true;
      }
    }
  }
  return false;
}

// main
int main() {
  /*
    Get the total supply cell ouput index and based on the index, 
    fetch the typeScript hash accompanied within the cell
  */
  int ret = 0;
  // remaining amount cell type script hash
  uint8_t rmng_amt_type_hash[BLAKE2B_BLOCK_SIZE];
  uint64_t len = BLAKE2B_BLOCK_SIZE;
  size_t find_ret = determine_rmng_amt_cell_output_index();
  if (find_ret == CKB_INDEX_OUT_OF_BOUND) {
    return ERROR_UNLOCK_FAIL;
  }

  ret = ckb_load_cell_by_field(rmng_amt_type_hash, &len, 0, find_ret, CKB_SOURCE_OUTPUT, CKB_CELL_FIELD_TYPE_HASH);
  if (ret != CKB_SUCCESS) {
    return ERROR_UNLOCK_FAIL;
  }

  size_t index = 0;
  while (index < SIZE_MAX) {
      uint64_t len = XUDT_HARDCAP_TYPE_SIZE;
      unsigned char type_script[XUDT_HARDCAP_TYPE_SIZE];
      ret = ckb_load_cell_by_field(type_script, &len, 0, index, CKB_SOURCE_OUTPUT, CKB_CELL_FIELD_TYPE);
      switch (ret) {
        case CKB_ITEM_MISSING:
          break;
        case CKB_SUCCESS:
          if (check_link(type_script, len, rmng_amt_type_hash)) {
            return CKB_SUCCESS;
          }
          break;
        default:
          return ERROR_UNLOCK_FAIL;
      }
      index++;
  }
  return ERROR_UNLOCK_FAIL;
}
