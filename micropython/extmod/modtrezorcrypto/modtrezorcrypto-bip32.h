/*
 * Copyright (c) Jan Pochyla, SatoshiLabs
 *
 * Licensed under TREZOR License
 * see LICENSE file for details
 */

#include "py/objstr.h"

#include "trezor-crypto/bip32.h"

typedef struct _mp_obj_HDNode_t {
    mp_obj_base_t base;
    uint32_t fingerprint;
    HDNode hdnode;
} mp_obj_HDNode_t;

STATIC const mp_obj_type_t mod_TrezorCrypto_HDNode_type;

#define XPUB_MAXLEN 128
#define ADDRESS_MAXLEN 36

/// def trezor.crypto.HDNode.derive(index: int) -> None:
///     '''
///     Derive a BIP0032 child node in place.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_derive(mp_obj_t self, mp_obj_t index) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);
    uint32_t i = mp_obj_get_int_truncated(index);
    uint32_t fp = hdnode_fingerprint(&o->hdnode);

    if (!hdnode_private_ckd(&o->hdnode, i)) {
        memset(&o->hdnode, 0, sizeof(o->hdnode));
        mp_raise_ValueError("Failed to derive");
    }
    o->fingerprint = fp;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_TrezorCrypto_HDNode_derive_obj, mod_TrezorCrypto_HDNode_derive);

/// def trezor.crypto.HDNode.derive_path(path: list) -> None:
///     '''
///     Go through a list of indexes and iteratively derive a child node in place.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_derive_path(mp_obj_t self, mp_obj_t path) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);

    // get path objects and length
    size_t plen;
    mp_obj_t *pitems;
    mp_obj_get_array(path, &plen, &pitems);
    if (plen > 32) {
        mp_raise_ValueError("Path cannot be longer than 32 indexes");
    }

    // convert path to int array
    uint32_t pi;
    uint32_t pints[plen];
    for (pi = 0; pi < plen; pi++) {
        if (!MP_OBJ_IS_INT(pitems[pi])) {
            mp_raise_TypeError("Index has to be int");
        }
        pints[pi] = mp_obj_get_int_truncated(pitems[pi]);
    }

    if (!hdnode_private_ckd_cached(&o->hdnode, pints, plen, &o->fingerprint)) {
        // derivation failed, reset the state and raise
        o->fingerprint = 0;
        memset(&o->hdnode, 0, sizeof(o->hdnode));
        mp_raise_ValueError("Failed to derive path");
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_TrezorCrypto_HDNode_derive_path_obj, mod_TrezorCrypto_HDNode_derive_path);

STATIC mp_obj_t serialize_public_private(mp_obj_t self, bool use_public) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);

    vstr_t vstr;
    vstr_init(&vstr, XPUB_MAXLEN);

    int written;
    if (use_public) {
        hdnode_fill_public_key(&o->hdnode);
        written = hdnode_serialize_public(&o->hdnode, o->fingerprint, vstr.buf, vstr.alloc);
    } else {
        written = hdnode_serialize_private(&o->hdnode, o->fingerprint, vstr.buf, vstr.alloc);
    }
    if (written <= 0) {
        mp_raise_ValueError("Failed to serialize");
    }
    vstr.len = written - 1; // written includes 0 at the end

    return mp_obj_new_str_from_vstr(&mp_type_str, &vstr);
}

/// def trezor.crypto.HDNode.serialize_public() -> str:
///     '''
///     Serialize the public info from HD node to base58 string.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_serialize_public(mp_obj_t self) {
    return serialize_public_private(self, true);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_TrezorCrypto_HDNode_serialize_public_obj, mod_TrezorCrypto_HDNode_serialize_public);

/// def trezor.crypto.HDNode.serialize_private() -> str:
///     '''
///     Serialize the private info HD node to base58 string.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_serialize_private(mp_obj_t self) {
    return serialize_public_private(self, false);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_TrezorCrypto_HDNode_serialize_private_obj, mod_TrezorCrypto_HDNode_serialize_private);

/// def trezor.crypto.HDNode.clone() -> HDNode:
///     '''
///     Returns a copy of the HD node.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_clone(mp_obj_t self) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);
    mp_obj_HDNode_t *copy = m_new_obj(mp_obj_HDNode_t);
    copy->base.type = &mod_TrezorCrypto_HDNode_type;
    copy->hdnode = o->hdnode;
    copy->fingerprint = o->fingerprint;
    return MP_OBJ_FROM_PTR(copy);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_TrezorCrypto_HDNode_clone_obj, mod_TrezorCrypto_HDNode_clone);

/// def trezor.crypto.HDNode.depth() -> int:
///     '''
///     Returns a depth of the HD node.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_depth(mp_obj_t self) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);
    return mp_obj_new_int_from_uint(o->hdnode.depth);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_TrezorCrypto_HDNode_depth_obj, mod_TrezorCrypto_HDNode_depth);

/// def trezor.crypto.HDNode.fingerprint() -> int:
///     '''
///     Returns a fingerprint of the HD node (hash of the parent public key).
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_fingerprint(mp_obj_t self) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);
    return mp_obj_new_int_from_uint(o->fingerprint);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_TrezorCrypto_HDNode_fingerprint_obj, mod_TrezorCrypto_HDNode_fingerprint);

/// def trezor.crypto.HDNode.child_num() -> int:
///     '''
///     Returns a child index of the HD node.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_child_num(mp_obj_t self) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);
    return mp_obj_new_int_from_uint(o->hdnode.child_num);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_TrezorCrypto_HDNode_child_num_obj, mod_TrezorCrypto_HDNode_child_num);

/// def trezor.crypto.HDNode.chain_code() -> bytes:
///     '''
///     Returns a chain code of the HD node.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_chain_code(mp_obj_t self) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);
    return mp_obj_new_str_of_type(&mp_type_bytes, o->hdnode.chain_code, sizeof(o->hdnode.chain_code));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_TrezorCrypto_HDNode_chain_code_obj, mod_TrezorCrypto_HDNode_chain_code);

/// def trezor.crypto.HDNode.private_key() -> bytes:
///     '''
///     Returns a private key of the HD node.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_private_key(mp_obj_t self) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);
    return mp_obj_new_str_of_type(&mp_type_bytes, o->hdnode.private_key, sizeof(o->hdnode.private_key));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_TrezorCrypto_HDNode_private_key_obj, mod_TrezorCrypto_HDNode_private_key);

/// def trezor.crypto.HDNode.public_key() -> bytes:
///     '''
///     Returns a public key of the HD node.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_public_key(mp_obj_t self) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);
    hdnode_fill_public_key(&o->hdnode);
    return mp_obj_new_str_of_type(&mp_type_bytes, o->hdnode.public_key, sizeof(o->hdnode.public_key));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_TrezorCrypto_HDNode_public_key_obj, mod_TrezorCrypto_HDNode_public_key);

/// def trezor.crypto.HDNode.address(version: int) -> str:
///     '''
///     Compute a base58-encoded address string from the HD node.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_HDNode_address(mp_obj_t self, mp_obj_t version) {
    mp_obj_HDNode_t *o = MP_OBJ_TO_PTR(self);

    uint32_t v = mp_obj_get_int_truncated(version);
    vstr_t vstr;
    vstr_init(&vstr, ADDRESS_MAXLEN);

    hdnode_get_address(&o->hdnode, v, vstr.buf, vstr.alloc);
    vstr.len = strlen(vstr.buf);
    return mp_obj_new_str_from_vstr(&mp_type_str, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_TrezorCrypto_HDNode_address_obj, mod_TrezorCrypto_HDNode_address);

STATIC const mp_rom_map_elem_t mod_TrezorCrypto_HDNode_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_derive), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_derive_obj) },
    { MP_ROM_QSTR(MP_QSTR_derive_path), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_derive_path_obj) },
    { MP_ROM_QSTR(MP_QSTR_serialize_private), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_serialize_private_obj) },
    { MP_ROM_QSTR(MP_QSTR_serialize_public), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_serialize_public_obj) },

    { MP_ROM_QSTR(MP_QSTR_clone), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_clone_obj) },
    { MP_ROM_QSTR(MP_QSTR_depth), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_depth_obj) },
    { MP_ROM_QSTR(MP_QSTR_fingerprint), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_fingerprint_obj) },
    { MP_ROM_QSTR(MP_QSTR_child_num), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_child_num_obj) },
    { MP_ROM_QSTR(MP_QSTR_chain_code), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_chain_code_obj) },
    { MP_ROM_QSTR(MP_QSTR_private_key), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_private_key_obj) },
    { MP_ROM_QSTR(MP_QSTR_public_key), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_public_key_obj) },
    { MP_ROM_QSTR(MP_QSTR_address), MP_ROM_PTR(&mod_TrezorCrypto_HDNode_address_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mod_TrezorCrypto_HDNode_locals_dict, mod_TrezorCrypto_HDNode_locals_dict_table);

STATIC const mp_obj_type_t mod_TrezorCrypto_HDNode_type = {
    { &mp_type_type },
    .name = MP_QSTR_HDNode,
    .locals_dict = (void*)&mod_TrezorCrypto_HDNode_locals_dict,
};

typedef struct _mp_obj_Bip32_t {
    mp_obj_base_t base;
} mp_obj_Bip32_t;

STATIC mp_obj_t mod_TrezorCrypto_Bip32_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    mp_obj_Bip32_t *o = m_new_obj(mp_obj_Bip32_t);
    o->base.type = type;
    return MP_OBJ_FROM_PTR(o);
}

/// def trezor.crypto.bip32.deserialize(value: str) -> HDNode:
///     '''
///     Construct a BIP0032 HD node from a base58-serialized value.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_Bip32_deserialize(mp_obj_t self, mp_obj_t value) {
    mp_buffer_info_t valueb;
    mp_get_buffer_raise(value, &valueb, MP_BUFFER_READ);
    if (valueb.len == 0) {
        mp_raise_ValueError("Invalid value");
    }
    vstr_t vstr;
    vstr_init_len(&vstr, valueb.len);
    memcpy(vstr.buf, valueb.buf, valueb.len);
    char *str = vstr_null_terminated_str(&vstr);

    HDNode hdnode;
    uint32_t fingerprint;
    if (hdnode_deserialize(str, &hdnode, &fingerprint) < 0) {
        mp_raise_ValueError("Failed to deserialize");
    }

    mp_obj_HDNode_t *o = m_new_obj(mp_obj_HDNode_t);
    o->base.type = &mod_TrezorCrypto_HDNode_type;
    o->hdnode = hdnode;
    o->fingerprint = fingerprint;
    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_TrezorCrypto_Bip32_deserialize_obj, mod_TrezorCrypto_Bip32_deserialize);

/// def trezor.crypto.bip32.from_seed(seed: bytes, curve_name: str) -> HDNode:
///     '''
///     Construct a BIP0032 HD node from a BIP0039 seed value.
///     '''
STATIC mp_obj_t mod_TrezorCrypto_Bip32_from_seed(mp_obj_t self, mp_obj_t seed, mp_obj_t curve_name) {
    mp_buffer_info_t seedb;
    mp_get_buffer_raise(seed, &seedb, MP_BUFFER_READ);
    if (seedb.len == 0) {
        mp_raise_ValueError("Invalid seed");
    }
    mp_buffer_info_t curveb;
    mp_get_buffer_raise(curve_name, &curveb, MP_BUFFER_READ);
    if (curveb.len == 0) {
        mp_raise_ValueError("Invalid curve name");
    }
    HDNode hdnode;
    if (!hdnode_from_seed(seedb.buf, seedb.len, curveb.buf, &hdnode)) {
        mp_raise_ValueError("Failed to derive the root node");
    }
    mp_obj_HDNode_t *o = m_new_obj(mp_obj_HDNode_t);
    o->base.type = &mod_TrezorCrypto_HDNode_type;
    o->hdnode = hdnode;
    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mod_TrezorCrypto_Bip32_from_seed_obj, mod_TrezorCrypto_Bip32_from_seed);

STATIC const mp_rom_map_elem_t mod_TrezorCrypto_Bip32_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deserialize), MP_ROM_PTR(&mod_TrezorCrypto_Bip32_deserialize_obj) },
    { MP_ROM_QSTR(MP_QSTR_from_seed), MP_ROM_PTR(&mod_TrezorCrypto_Bip32_from_seed_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mod_TrezorCrypto_Bip32_locals_dict, mod_TrezorCrypto_Bip32_locals_dict_table);

STATIC const mp_obj_type_t mod_TrezorCrypto_Bip32_type = {
    { &mp_type_type },
    .name = MP_QSTR_Bip32,
    .make_new = mod_TrezorCrypto_Bip32_make_new,
    .locals_dict = (void*)&mod_TrezorCrypto_Bip32_locals_dict,
};
