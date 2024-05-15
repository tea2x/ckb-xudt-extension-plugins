// #define CKB_C_STDLIB_PRINTF
// #include <stdio.h>

#include "blockchain.h"
#include "ckb_syscalls.h"

// We will leverage gcc's 128-bit integer extension here for number crunching.
typedef unsigned __int128 uint128_t;

// Common error codes that might be returned by the script.
#define ERROR_NO_HARDCAP_CELL 1
#define ERROR_CAN_NOT_MINT_OVER_HARDCAP 2
#define ERROR_WRONG_HARDCAP_CALCULATION 3

// The modes of operation for the script. 
enum Mode
{
	Burn, // Consume/burn existing tokens.
	Create, // Create new tokens.
	Transfer, // Transfer (update) tokens
};

// sum xudt amount
static uint128_t sum_xudt(size_t source) {
    uint128_t output_amount = 0;
    size_t counter = 0;
    uint64_t len = 16;
    while (1) {
        uint128_t current_amount = 0;
        int ret = ckb_load_cell_data((uint8_t *)&current_amount, &len, 0, counter, source);
        if (ret == CKB_INDEX_OUT_OF_BOUND) {
            break;
        }
        output_amount += current_amount;
        counter += 1;
    }
    return output_amount;
}

// determines the mode of operation for the currently executing script.
static uint8_t determine_mode() {
	// Gather counts on the number of group input and groupt output cells.
	uint128_t input_amount_count = sum_xudt(CKB_SOURCE_GROUP_INPUT);
	uint128_t output_amount_count = sum_xudt(CKB_SOURCE_GROUP_OUTPUT);

	// Detect the operation based on the cell count.
	if (input_amount_count > output_amount_count) {
		return Burn;
	}
	if (input_amount_count < output_amount_count) {
		return Create;
	}
	return Transfer;
}

// check if hard_cap cell exists in the source list
static size_t find_hard_capped_cell(uint8_t *code_hash_ptr, size_t source) {
    int ret = 0;
    size_t current = 0;
    while (current < SIZE_MAX) {
        uint64_t len = 32;
        uint8_t hash[32];
        ret = ckb_load_cell_by_field(hash, &len, 0, current, source, CKB_CELL_FIELD_TYPE_HASH);
        switch (ret) {
            case CKB_ITEM_MISSING:
                break;
            case CKB_SUCCESS:
                if (memcmp(code_hash_ptr, hash, 32) == 0) {
                    /* Found a match */
                    return current;
                }
                break;
            default:
                return ERROR_NO_HARDCAP_CELL;
        }
        current++;
    }
    return ERROR_NO_HARDCAP_CELL;
}

// DLL public interface
__attribute__((visibility("default"))) int validate(int owner_mode, uint32_t i, uint8_t * args_ptr, uint32_t args_size) {
    /* with sudt, owner mode is god mode and is CREATION/BURN mode.
       With this script hard_cap, in owner_mode + CREATION, it must go through the following validation.
    */ 
    enum Mode operation_mode = determine_mode();
    if (!(owner_mode && operation_mode == Create))
        return CKB_SUCCESS;

    // find the hardcap(a typeId) cell in the inputs
    uint32_t old_hard_cap = 0;
    uint64_t len = 4;
    size_t find_ret;
    find_ret = find_hard_capped_cell(args_ptr, CKB_SOURCE_INPUT);
    if (find_ret == ERROR_NO_HARDCAP_CELL)
        return find_ret;
    ckb_load_cell_data((uint8_t *)&old_hard_cap, &len, 0, find_ret, CKB_SOURCE_INPUT);

    // find the hardcap(a typeId) cell in the outputs
    uint32_t new_hard_cap = 0;
    find_ret = find_hard_capped_cell(args_ptr, CKB_SOURCE_OUTPUT);
    if (find_ret == ERROR_NO_HARDCAP_CELL)
        return find_ret;
    ckb_load_cell_data((uint8_t *)&new_hard_cap, &len, 0, find_ret, CKB_SOURCE_OUTPUT);

    // fetch and sum all xudt amount in the outputs
    uint128_t output_amount = sum_xudt(CKB_SOURCE_GROUP_OUTPUT);
    
    // validate hardcap rules
    if (output_amount > (uint128_t)old_hard_cap)
        return ERROR_CAN_NOT_MINT_OVER_HARDCAP;
    
    if ((uint128_t)new_hard_cap != ((uint128_t)old_hard_cap - output_amount))
        return ERROR_WRONG_HARDCAP_CALCULATION;

    return CKB_SUCCESS;
}
