/******************************************************************************
 *
 * Copyright 2017 Xaptum, Inc.
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

#include "ecdaa-test-utils.h"

#include "src/amcl-extensions/big_XXX.h"
#include "src/amcl-extensions/ecp_ZZZ.h"
#include "src/amcl-extensions/ecp2_ZZZ.h"

#include <ecdaa/member_keypair_ZZZ.h>
#include <ecdaa/credential_ZZZ.h>
#include <ecdaa/issuer_keypair_ZZZ.h>
#include <ecdaa/signature_ZZZ.h>
#include <ecdaa/group_public_key_ZZZ.h>
#include <ecdaa/revocation_list_ZZZ.h>
#include <ecdaa/prng.h>

#include <string.h>

#include <sys/time.h>

static void sign_then_verify_good();
static void sign_then_verify_on_rev_list();
static void lengths_same();
static void serialize_deserialize();
static void deserialize_garbage_fails();

typedef struct sign_and_verify_fixture {
    struct ecdaa_prng prng;
    uint8_t *msg;
    uint32_t msg_len;
    struct ecdaa_revocation_list_ZZZ sk_rev_list;
    struct ecdaa_member_public_key_ZZZ pk;
    struct ecdaa_member_secret_key_ZZZ sk;
    struct ecdaa_issuer_public_key_ZZZ ipk;
    struct ecdaa_issuer_secret_key_ZZZ isk;
    struct ecdaa_credential_ZZZ cred;
} sign_and_verify_fixture;

static void setup(sign_and_verify_fixture* fixture);
static void teardown(sign_and_verify_fixture *fixture);

int main()
{
    sign_then_verify_good();
    sign_then_verify_on_rev_list();
    lengths_same();
    serialize_deserialize();
    deserialize_garbage_fails();
}

static void setup(sign_and_verify_fixture* fixture)
{
    TEST_ASSERT(0 == ecdaa_prng_init(&fixture->prng));

    big_XXX_random_mod_order(&fixture->isk.x, get_csprng(&fixture->prng));
    ecp2_ZZZ_set_to_generator(&fixture->ipk.gpk.X);
    ECP2_ZZZ_mul(&fixture->ipk.gpk.X, fixture->isk.x);

    big_XXX_random_mod_order(&fixture->isk.y, get_csprng(&fixture->prng));
    ecp2_ZZZ_set_to_generator(&fixture->ipk.gpk.Y);
    ECP2_ZZZ_mul(&fixture->ipk.gpk.Y, fixture->isk.y);

    ecp_ZZZ_set_to_generator(&fixture->pk.Q);
    big_XXX_random_mod_order(&fixture->sk.sk, get_csprng(&fixture->prng));
    ECP_ZZZ_mul(&fixture->pk.Q, fixture->sk.sk);

    struct ecdaa_credential_ZZZ_signature cred_sig;
    ecdaa_credential_ZZZ_generate(&fixture->cred, &cred_sig, &fixture->isk, &fixture->pk, &fixture->prng);

    fixture->msg = (uint8_t*) "Test message";
    fixture->msg_len = (uint32_t)strlen((char*)fixture->msg);

    fixture->sk_rev_list.length=0;
    fixture->sk_rev_list.list=NULL;
}

static void teardown(sign_and_verify_fixture *fixture)
{
    ecdaa_prng_free(&fixture->prng);
}

static void sign_then_verify_good()
{
    printf("Starting signature::sign_then_verify_good...\n");

    sign_and_verify_fixture fixture;
    setup(&fixture);

    struct ecdaa_signature_ZZZ sig;
    TEST_ASSERT(0 == ecdaa_signature_ZZZ_sign(&sig, fixture.msg, fixture.msg_len, &fixture.sk, &fixture.cred, &fixture.prng));

    TEST_ASSERT(0 == ecdaa_signature_ZZZ_verify(&sig, &fixture.ipk.gpk, &fixture.sk_rev_list, fixture.msg, fixture.msg_len));

    teardown(&fixture);

    printf("\tsuccess\n");
}

static void sign_then_verify_on_rev_list()
{
    printf("Starting signature::sign_then_verify_on_rev_list...\n");

    sign_and_verify_fixture fixture;
    setup(&fixture);

    // Put self on a secret-key revocation list, to be used in verify.
    struct ecdaa_member_secret_key_ZZZ sk_rev_list_bad_raw[1];
    BIG_XXX_copy(sk_rev_list_bad_raw[0].sk, fixture.sk.sk);
    struct ecdaa_revocation_list_ZZZ sk_rev_list_bad = {.length=1, .list=sk_rev_list_bad_raw};

    struct ecdaa_signature_ZZZ sig;
    TEST_ASSERT(0 == ecdaa_signature_ZZZ_sign(&sig, fixture.msg, fixture.msg_len, &fixture.sk, &fixture.cred, &fixture.prng));

    TEST_ASSERT(0 != ecdaa_signature_ZZZ_verify(&sig, &fixture.ipk.gpk, &sk_rev_list_bad, fixture.msg, fixture.msg_len));

    teardown(&fixture);

    printf("\tsuccess\n");
}

static void lengths_same()
{
    printf("Starting signature::lengths_same...\n");

    TEST_ASSERT(ECDAA_SIGNATURE_ZZZ_LENGTH == ecdaa_signature_ZZZ_length());

    printf("\tsuccess\n");
}

static void serialize_deserialize()
{
    printf("Starting signature::serialize_deserialize...\n");

    sign_and_verify_fixture fixture;
    setup(&fixture);

    struct ecdaa_signature_ZZZ sig;
    TEST_ASSERT(0 == ecdaa_signature_ZZZ_sign(&sig, fixture.msg, fixture.msg_len, &fixture.sk, &fixture.cred, &fixture.prng));

    TEST_ASSERT(0 == ecdaa_signature_ZZZ_verify(&sig, &fixture.ipk.gpk, &fixture.sk_rev_list, fixture.msg, fixture.msg_len));

    uint8_t buffer[ECDAA_SIGNATURE_ZZZ_LENGTH];
    ecdaa_signature_ZZZ_serialize(buffer, &sig);
    struct ecdaa_signature_ZZZ sig_deserialized;
    TEST_ASSERT(0 == ecdaa_signature_ZZZ_deserialize(&sig_deserialized, buffer));
    TEST_ASSERT(0 == ecdaa_signature_ZZZ_deserialize_and_verify(&sig_deserialized, &fixture.ipk.gpk, &fixture.sk_rev_list, buffer, fixture.msg, fixture.msg_len));

    teardown(&fixture);

    printf("\tsuccess\n");
}

static void deserialize_garbage_fails()
{
    printf("Starting signature::serialize_deserialize...\n");

    uint8_t buffer[ECDAA_SIGNATURE_ZZZ_LENGTH] = {0};
    struct ecdaa_signature_ZZZ sig_deserialized;
    TEST_ASSERT(0 != ecdaa_signature_ZZZ_deserialize(&sig_deserialized, buffer));

    printf("\tsuccess\n");
}