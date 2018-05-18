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

#include "ecdaa-benchmark-utils.h"

#include "schnorr/schnorr_ZZZ.h"
#include "amcl-extensions/big_XXX.h"
#include "amcl-extensions/ecp_ZZZ.h"
#include "amcl-extensions/ecp2_ZZZ.h"

#include <ecdaa/credential_ZZZ.h>
#include <ecdaa/member_keypair_ZZZ.h>
#include <ecdaa/credential_ZZZ.h>
#include <ecdaa/issuer_keypair_ZZZ.h>
#include <ecdaa/signature_ZZZ.h>
#include <ecdaa/group_public_key_ZZZ.h>
#include <ecdaa/revocations_ZZZ.h>
#include <ecdaa/rand.h>

#include <sys/time.h>
#include <string.h>

static void schnorr_sign_benchmark();

static void sign_benchmark();
static void verify_benchmark();

typedef struct sign_and_verify_fixture {
    uint8_t *msg;
    uint32_t msg_len;
    uint8_t *basename;
    uint32_t basename_len;
    struct ecdaa_revocations_ZZZ revocations;
    struct ecdaa_member_public_key_ZZZ pk;
    struct ecdaa_member_secret_key_ZZZ sk;
    struct ecdaa_issuer_public_key_ZZZ ipk;
    struct ecdaa_issuer_secret_key_ZZZ isk;
    struct ecdaa_credential_ZZZ cred;
} sign_and_verify_fixture;

static void setup(sign_and_verify_fixture* fixture);
static void teardown(sign_and_verify_fixture* fixture);

int main()
{
    schnorr_sign_benchmark();

    sign_benchmark();
    verify_benchmark();
}

static void setup(sign_and_verify_fixture* fixture)
{
    ecp_ZZZ_random_mod_order(&fixture->isk.x, benchmark_randomness);
    ecp2_ZZZ_set_to_generator(&fixture->ipk.gpk.X);
    ECP2_ZZZ_mul(&fixture->ipk.gpk.X, fixture->isk.x);

    ecp_ZZZ_random_mod_order(&fixture->isk.y, benchmark_randomness);
    ecp2_ZZZ_set_to_generator(&fixture->ipk.gpk.Y);
    ECP2_ZZZ_mul(&fixture->ipk.gpk.Y, fixture->isk.y);

    ecp_ZZZ_set_to_generator(&fixture->pk.Q);
    ecp_ZZZ_random_mod_order(&fixture->sk.sk, benchmark_randomness);
    ECP_ZZZ_mul(&fixture->pk.Q, fixture->sk.sk);

    struct ecdaa_credential_ZZZ_signature cred_sig;
    ecdaa_credential_ZZZ_generate(&fixture->cred, &cred_sig, &fixture->isk, &fixture->pk, benchmark_randomness);

    fixture->msg = (uint8_t*) "Test message";
    fixture->msg_len = strlen((char*)fixture->msg);

    fixture->basename = (uint8_t*) "BASENAME";
    fixture->basename_len = (uint32_t)strlen((char*)fixture->basename);

    fixture->revocations.sk_length=0;
    fixture->revocations.sk_list=NULL;
    fixture->revocations.bsn_length=0;
    fixture->revocations.bsn_list=NULL;
}

static void teardown(sign_and_verify_fixture* fixture)
{
    (void)fixture;
}

void schnorr_sign_benchmark()
{
    unsigned rounds = 2500;

    printf("Starting schnorr::schnorr_sign_benchmark (%u iterations)...\n", rounds);

    ECP_ZZZ public;
    BIG_XXX private;

    schnorr_keygen_ZZZ(&public, &private, benchmark_randomness);

    uint8_t *msg = (uint8_t*) "Test message";
    uint32_t msg_len = strlen((char*)msg);

    BIG_XXX c, s;

    struct timeval tv1;
    gettimeofday(&tv1, NULL);

    ECP_ZZZ basepoint;
    ecp_ZZZ_set_to_generator(&basepoint);
    for (unsigned i = 0; i < rounds; i++) {
        schnorr_sign_ZZZ(&c, &s, NULL, msg, msg_len, &basepoint, &public, private, NULL, 0, benchmark_randomness);
    }

    struct timeval tv2;
    gettimeofday(&tv2, NULL);
    unsigned long long elapsed = (tv2.tv_usec + tv2.tv_sec * 1000000) -
        (tv1.tv_usec + tv1.tv_sec * 1000000);

    printf("%llu usec (%6llu signs/s)\n",
            elapsed,
            rounds * 1000000ULL / elapsed);
}

static void sign_benchmark()
{
    unsigned rounds = 250;

    printf("Starting sign-and-verify::sign_benchmark (%u iterations)...\n", rounds);

    sign_and_verify_fixture fixture;
    setup(&fixture);

    struct ecdaa_signature_ZZZ sig;

    struct timeval tv1;
    gettimeofday(&tv1, NULL);

    for (unsigned i = 0; i < rounds; i++) {
        BENCHMARK_ASSERT(0 == ecdaa_signature_ZZZ_sign(&sig, fixture.msg, fixture.msg_len, fixture.basename, fixture.basename_len, &fixture.sk, &fixture.cred, benchmark_randomness));
    }

    struct timeval tv2;
    gettimeofday(&tv2, NULL);
    unsigned long long elapsed = (tv2.tv_usec + tv2.tv_sec * 1000000) -
        (tv1.tv_usec + tv1.tv_sec * 1000000);

    teardown(&fixture);

    printf("%llu usec (%6llu signs/s)\n",
            elapsed,
            rounds * 1000000ULL / elapsed);
}

static void verify_benchmark()
{
    unsigned rounds = 250;

    printf("Starting sign-and-verify::verify_benchmark (%u iterations)...\n", rounds);

    sign_and_verify_fixture fixture;
    setup(&fixture);

    struct ecdaa_signature_ZZZ sig;

    BENCHMARK_ASSERT(0 == ecdaa_signature_ZZZ_sign(&sig, fixture.msg, fixture.msg_len, fixture.basename, fixture.basename_len, &fixture.sk, &fixture.cred, benchmark_randomness));

    struct timeval tv1;
    gettimeofday(&tv1, NULL);

    for (unsigned i = 0; i < rounds; i++) {
        BENCHMARK_ASSERT(0 == ecdaa_signature_ZZZ_verify(&sig, &fixture.ipk.gpk, &fixture.revocations, fixture.msg, fixture.msg_len, fixture.basename, fixture.basename_len));
    }

    struct timeval tv2;
    gettimeofday(&tv2, NULL);
    unsigned long long elapsed = (tv2.tv_usec + tv2.tv_sec * 1000000) -
        (tv1.tv_usec + tv1.tv_sec * 1000000);

    teardown(&fixture);

    printf("%llu usec (%6llu verifications/s)\n",
            elapsed,
            rounds * 1000000ULL / elapsed);
}
