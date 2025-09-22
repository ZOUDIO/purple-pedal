#ifndef ZEPHYR_INCLUDE_APP_LIB_P1METER_H_
#define ZEPHYR_INCLUDE_APP_LIB_P1METER_H_

#include <zephyr/sys/util.h>
#include <stddef.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


enum p1_obj_type{
    P1_OBJ_TYPE_DESCR = 0,
    P1_OBJ_TYPE_OBIS = 1,
    P1_OBJ_TYPE_CRC = 2,
};

struct p1_obj_descr{
    union {
        struct {
                uint8_t grp_a;
                uint8_t grp_b;
                uint8_t grp_c;
                uint8_t grp_d;
                uint8_t grp_e;
        }obis;
        uint64_t obis_raw;
    };
    uint32_t offset;    //offset of the destination memory
    uint32_t size;      //size of the destination memory
};

#define P1_OBIS_STR(_a,_b,_c,_d,_e)  "\""#_a"-"#_b":"#_c"."#_d"."#_e"\""

#define P1_OBJ_DESC(obis_, struct_, field_name_) \
    { \
        .obis = obis_, \
        .offset = offsetof(struct_, field_name_), \
        .size = SIZEOF_FIELD(struct_, field_name_), \
    }


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_APP_LIB_P1METER_H_ */