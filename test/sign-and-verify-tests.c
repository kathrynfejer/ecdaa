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

#include "xaptum-test-utils.h"

#include <xaptum-ecdaa/member.h>
#include <xaptum-ecdaa/verify.h>
#include <xaptum-ecdaa/issuer_nonce.h>
#include <xaptum-ecdaa/issuer_keypair.h>
#include <xaptum-ecdaa/signature.h>
#include <xaptum-ecdaa/group_public_key.h>
#include <xaptum-ecdaa/sk_revocation_list.h>

#include <string.h>

#include <sys/time.h>

static void sign_then_verify_good();
static void sign_then_verify_on_rev_list();
static void sign_benchmark();
static void verify_benchmark();

typedef struct sign_and_verify_fixture {
    ecdaa_group_public_key_t gpk;
    uint8_t *msg;
    uint32_t msg_len;
    ecdaa_member_t member;
    ecdaa_sk_revocation_list_t sk_rev_list;
} sign_and_verify_fixture;

static void setup(sign_and_verify_fixture* fixture);

int main()
{
    sign_then_verify_good();
    sign_then_verify_on_rev_list();
    sign_benchmark();
    verify_benchmark();
}

static void setup(sign_and_verify_fixture* fixture)
{
    create_test_rng(&fixture->member.rng);

    ecdaa_issuer_public_key_t ipk;
    ecdaa_issuer_secret_key_t isk;
    ecdaa_generate_issuer_key_pair(&ipk, &isk, &fixture->member.rng);
    memcpy(&fixture->gpk, &ipk.gpk, sizeof(ecdaa_group_public_key_t));

    ecdaa_issuer_nonce_t nonce = {{0}};
    ecdaa_generate_member_key_pair(&fixture->member.pk, &fixture->member.sk, &nonce, &fixture->member.rng);

    ecdaa_credential_signature_t cred_sig;
    ecdaa_generate_credential(&fixture->member.cred, &cred_sig, &isk, &fixture->member.pk, &fixture->member.rng);

    fixture->msg = (uint8_t*) "Test message";
    fixture->msg_len = strlen((char*)fixture->msg);

    fixture->sk_rev_list.length=0;
    fixture->sk_rev_list.list=NULL;
}

static void sign_then_verify_good()
{
    printf("Starting sign-and-verify::sign_then_verify_good...\n");

    sign_and_verify_fixture fixture;
    setup(&fixture);

    ecdaa_signature_t sig;
    TEST_ASSERT(0 == ecdaa_sign(&sig, fixture.msg, fixture.msg_len, &fixture.member));

    TEST_ASSERT(0 == ecdaa_verify(&sig, &fixture.gpk, &fixture.sk_rev_list, fixture.msg, fixture.msg_len));

    printf("\tsuccess\n");
}

static void sign_then_verify_on_rev_list()
{
    printf("Starting sign-and-verify::sign_then_verify_on_rev_list...\n");

    sign_and_verify_fixture fixture;
    setup(&fixture);

    // Put self on a secret-key revocation list, to be used in verify.
    ecdaa_member_secret_key_t sk_rev_list_bad_raw[1];
    BIG_256_56_copy(sk_rev_list_bad_raw[0].sk, fixture.member.sk.sk);
    ecdaa_sk_revocation_list_t sk_rev_list_bad = {.length=1, .list=sk_rev_list_bad_raw};

    ecdaa_signature_t sig;
    TEST_ASSERT(0 == ecdaa_sign(&sig, fixture.msg, fixture.msg_len, &fixture.member));

    TEST_ASSERT(0 != ecdaa_verify(&sig, &fixture.gpk, &sk_rev_list_bad, fixture.msg, fixture.msg_len));

    printf("\tsuccess\n");
}

static void sign_benchmark()
{
    unsigned rounds = 250;

    printf("Starting sign-and-verify::sign_benchmark (%d iterations)...\n", rounds);

    sign_and_verify_fixture fixture;
    setup(&fixture);

    ecdaa_signature_t sig;

    struct timeval tv1;
    gettimeofday(&tv1, NULL);

    for (unsigned i = 0; i < rounds; i++) {
        TEST_ASSERT(0 == ecdaa_sign(&sig, fixture.msg, fixture.msg_len, &fixture.member));
    }

    struct timeval tv2;
    gettimeofday(&tv2, NULL);
    unsigned long long elapsed = (tv2.tv_usec + tv2.tv_sec * 1000000) -
        (tv1.tv_usec + tv1.tv_sec * 1000000);

    printf("%llu usec (%6llu signs/s)\n",
            elapsed,
            rounds * 1000000ULL / elapsed);
}

static void verify_benchmark()
{
    unsigned rounds = 250;

    printf("Starting sign-and-verify::verify_benchmark (%d iterations)...\n", rounds);

    sign_and_verify_fixture fixture;
    setup(&fixture);

    ecdaa_signature_t sig;

    TEST_ASSERT(0 == ecdaa_sign(&sig, fixture.msg, fixture.msg_len, &fixture.member));

    struct timeval tv1;
    gettimeofday(&tv1, NULL);

    for (unsigned i = 0; i < rounds; i++) {
        TEST_ASSERT(0 == ecdaa_verify(&sig, &fixture.gpk, &fixture.sk_rev_list, fixture.msg, fixture.msg_len));
    }

    struct timeval tv2;
    gettimeofday(&tv2, NULL);
    unsigned long long elapsed = (tv2.tv_usec + tv2.tv_sec * 1000000) -
        (tv1.tv_usec + tv1.tv_sec * 1000000);

    printf("%llu usec (%6llu verifications/s)\n",
            elapsed,
            rounds * 1000000ULL / elapsed);
}
