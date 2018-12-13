/******************************************************************************
 *
 * Copyright 2018 Xaptum, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License
 *
 *****************************************************************************/
#include "ecdaa_issuer_respond_to_join_request.h"
#include "tool_rand.h"
#include <ecdaa.h>
#include <string.h>
#include <stdint.h>

int ecdaa_issuer_respond_to_join_request(const char* member_public_key_file,
                                        const char* issuer_secret_key_file,
                                        const char* credential_out_file,
                                        const char* credential_signature_out_file,
                                        const char* nonce)
{
    // Read member public key from disk.
    // NOTE: If this Join procedure is being done remotely,
    //  there should be some way of authenticating this member's public key.
    //  For our purposes, we assume this is an "in-factory" join,
    //  and so the authenticity of this member is ensured
    //  via physical means.
    size_t nonce_len = strlen(nonce);
    if (nonce_len > 1048576) {    // 1MiB
        return NONCE_OVERFLOW;
    }
    struct ecdaa_member_public_key_FP256BN pk;
    int ret = ecdaa_member_public_key_FP256BN_deserialize_file(&pk, member_public_key_file, (uint8_t*)nonce, (uint32_t)nonce_len);
    if (0 != ret)
        return ret;

    // Read issuer secret key from disk;
    struct ecdaa_issuer_secret_key_FP256BN isk;
    ret = ecdaa_issuer_secret_key_FP256BN_deserialize_file(&isk, issuer_secret_key_file);
    if (0 != ret)
        return ret;

    // Generate new credential for this member, along with a credential signature.
    struct ecdaa_credential_FP256BN cred;
    struct ecdaa_credential_FP256BN_signature cred_sig;
    ret = ecdaa_credential_FP256BN_generate(&cred, &cred_sig, &isk, &pk, tool_rand);
    if (0 != ret) {
        return CRED_CREATION_ERROR;
    }

    // Write credential to file
    ret = ecdaa_credential_FP256BN_serialize_file(credential_out_file, &cred);
    if (0 != ret) {
        return ret;
    }

    // Write credential signature to file
    ret = ecdaa_credential_FP256BN_signature_serialize_file(credential_signature_out_file, &cred_sig);
    if (0 != ret) {
        return ret;
    }

    return SUCCESS;
}
