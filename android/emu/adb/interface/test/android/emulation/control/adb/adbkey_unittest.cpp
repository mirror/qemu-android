// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/emulation/control/adb/adbkey.h"
#include <gtest/gtest.h>
#include <openssl/obj_mac.h>
#include <openssl/rsa.h>
#include <memory>
#include <fstream>
#include <string>
#include <vector>
#include "aemu/base/files/PathUtils.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

namespace android {
namespace base {

// Secret token that gets verified.
const uint8_t challenge_token[20] = {0xE8, 0x99, 0xE1, 0xFF, 0x95, 0x9B, 0x8F,
                                     0x6B, 0x54, 0xBE, 0xCE, 0xC5, 0x42, 0xF1,
                                     0x93, 0x7D, 0x3,  0x99, 0xA2, 0x32};

// Private key that can be used to answer challenge token above.
std::string privkey = R"##(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCxc0jUMYZ5fJSo
7t0MEfGDV9GPhdvOfgKv3GRaqnPuTD04SCXPwu7iJ9y6rS/Rk1CsW9HI+LcQvu5h
AsbpzDW88Sg5kSWfbEWP7EgPfdcfWdOfaRFZ/0J/sRejvCnekmnPF75irEYPI/sx
jL2xIJ1kTOywOCBB3l/xkPPSLPt2159Lm3v+pMdinq6rp8RQ7XK9pj4dv4tb778+
mLtp4p6FhOH6rU/q4848v4CM1/YVXZZaMKmos4PfvFLknzCmd3xN2o1MusP2OSxb
NkXXpCTsJLufcb3ItBgLyEri/eOQcYzpV84vTHwqr121stDpjVAR3H4AaelHZqOF
JKY5NVJTAgMBAAECggEASD7BdfK75xY7iBPH1zQu+eR1I1PCS+2ttl+qU+d1z50m
h5WIH3Ajxduo2C/OeirZ+3JelM396klxz/lLdsB3WHdugxF/GcsA/zmZlQUM4my1
5f7m25c7QbWeBEGFYmKFxZTLJG0zENL7YA8G4+h9a+qNqqkPKQIaWcVEH1vE/Xra
hNwD6wYyj01aVAdSDKgu6EtIpmVDlgDFE33JQnHM9r3c6o6QdNQTlhGyQ+SX2rCx
y3a0pW4MlGas1RtWDwOHt6XunSn2O6YUGuEXLhixFxv/rhr3+x5TSFcHB9Lg8/Rm
OEvW9614/C0ePkDuoxcdGbauMz1JU4O0xkCQameJIQKBgQDYCjmQJ3s55G6nIrOe
uE5R8m1QpbZhg2w/8J5dUirsidw7WG5Wo3bnd5RTciti59r7TUUD/XS7BkIUiRZk
GKHvC+W2QJT3u7oCY2KGXhCAniwbckHLsVnfPMqKyN+LKU8SbXqos9pZUz7nAmvG
G7FVrrd2lYnjDSgMMPr+8cO/CQKBgQDSRcp0jUifpoYOnyO3Lt50s94Y4J3SEDJW
N/DXtck9WOjDuGA379KZV2BTkBVKoZQ3a3qamHjGBkClSg01/no0jhAuieMt8UTG
NvKGx4Ec8pBOkJleeDwNkcaC62AFjlz2+aWe4FFpnSRp7S20R+dMOGTLc+YaD6xo
JLvi2f2BewKBgBNSDsXSkhWiVTcDRncKWo6/lIEi4MWlwDeTqEYGRCp1RcnU5cE/
yzF2I0C3NCQbQh05UtPBhf/31k8J14PKJClBsiBzdB8XndH622PS47zs6FroA/RY
fwYU5LQ2tK84WYb3XYHa28sjQ7vbHpJQBbL49hVX2EYC9jLo6nmEW5IpAoGABAoZ
MJICQiblzmQaQIui9GT8MEgoX/+1p9hdRReV7RrHJfNlzc1Ko219ST2sWwmtmj7z
VQL21v8JwOMiS9Y+rMHJ58r4VUqcQp6NnC86+L5kLU4z1A/FP5F8WcmBx7mLaac0
GlA+4COHro1C4oK7G8i9jvcEBZ4ldr616U68wv8CgYEAgH2P+jwmAOiEDGJIvTOP
IPkwJ9Kz2z6z4JxfQxfo2sdM8aCrgsXvfFIFZpAWZ13HXzKFNjfdZ3tNlJOuiiEP
Nw3rvNsjZXrpaIYpIKHL/QJmRM7MO1El4ebV3/qBZl0NU33fguD4P3FmqZmh2BvJ
Uauxw86jrPSgjbiRB8FyvTo=
-----END PRIVATE KEY-----)##";

class AdbKeyTest : public ::testing::Test {
public:
    AdbKeyTest() {
        mTestDir = mTestSystem.getTempRoot();
        mTestDir->makeSubDir(".android");
        mTestSystem.setHomeDirectory(mTestDir->pathString());
    }

    ~AdbKeyTest() {}

protected:
    TestSystem mTestSystem{"/", System::kProgramBitness};
    TestTempDir* mTestDir;
    const char* tstKey = "adbsamplekey";
};

TEST_F(AdbKeyTest, key_does_not_exist) {
    EXPECT_EQ("", getAdbKeyPath("thisshouldnotexist.boo"));
}

TEST_F(AdbKeyTest, find_keys_in_default_path) {
    mTestDir->makeSubFile(".android/adbkey");
    mTestDir->makeSubFile(".android/adbkey.pub");
    EXPECT_NE("", getPrivateAdbKeyPath());
    EXPECT_NE("", getPublicAdbKeyPath());
}

TEST_F(AdbKeyTest, generate_writes_a_key) {
    std::string keyFile = pj({mTestDir->pathString(), ".android", tstKey});
    EXPECT_EQ("", getAdbKeyPath(tstKey));
    EXPECT_TRUE(adb_auth_keygen(keyFile.c_str()));
    EXPECT_NE("", getAdbKeyPath(tstKey));
}

TEST_F(AdbKeyTest, can_create_pub_from_generated_priv) {
    std::string pubkey;
    std::string keyFile = pj({mTestDir->pathString(), ".android", tstKey});
    EXPECT_TRUE(adb_auth_keygen(keyFile.c_str()));
    EXPECT_TRUE(pubkey_from_privkey(keyFile, &pubkey));
    EXPECT_NE("", pubkey);
}

TEST_F(AdbKeyTest, can_sign_token) {
    std::string pubkey;
    std::string keyFile = pj({mTestDir->pathString(), ".android", kPrivateKeyFileName});
    std::ofstream out(keyFile);
    out << privkey << std::endl;
    out.close();


    EXPECT_TRUE(pubkey_from_privkey(keyFile, &pubkey));
    EXPECT_NE("", pubkey);

    int siglen = 256;
    std::vector<uint8_t> signed_token{};
    signed_token.resize(siglen);
    EXPECT_TRUE(sign_auth_token(challenge_token, sizeof(challenge_token),
                                signed_token.data(), siglen));

    EXPECT_NE("", std::string((char*)signed_token.data(), siglen));
}

// Test digest to verify.
const uint8_t kDigest[] = {
        0x31, 0x5f, 0x5b, 0xdb, 0x76, 0xd0, 0x78, 0xc4, 0x3b, 0x8a, 0xc0,
        0x06, 0x4e, 0x4a, 0x01, 0x64, 0x61, 0x2b, 0x1f, 0xce, 0x77, 0xc8,
        0x69, 0x34, 0x5b, 0xfc, 0x94, 0xc7, 0x58, 0x94, 0xed, 0xd3,
};

// 2048 RSA test key.
const uint8_t kKey2048[ANDROID_PUBKEY_ENCODED_SIZE] = {
        0x40, 0x00, 0x00, 0x00, 0x05, 0x75, 0x61, 0xd1, 0x33, 0xf0, 0x2d, 0x12,
        0x45, 0xfb, 0xae, 0x07, 0x02, 0x15, 0x4f, 0x3a, 0x2b, 0xa3, 0xbc, 0x49,
        0xbd, 0x14, 0x07, 0xa0, 0xc0, 0x9f, 0x0c, 0x52, 0x60, 0x77, 0x9f, 0xa2,
        0x31, 0xd0, 0xa7, 0xfb, 0x7e, 0xde, 0xfb, 0xc9, 0x05, 0xc0, 0x97, 0xf7,
        0x74, 0x99, 0xe6, 0xd1, 0x08, 0xa6, 0xc2, 0x59, 0x5a, 0xd8, 0x37, 0x1d,
        0xe0, 0x48, 0x5e, 0x63, 0x44, 0x04, 0x8b, 0x05, 0x20, 0xf6, 0x25, 0x67,
        0x38, 0xb2, 0xb6, 0xf9, 0xbe, 0xb6, 0x1d, 0x7f, 0x1b, 0x71, 0x8a, 0xeb,
        0xb7, 0xf8, 0x01, 0xc1, 0x5e, 0xf7, 0xfe, 0x48, 0x08, 0x27, 0x0f, 0x27,
        0x2a, 0x64, 0x1a, 0x43, 0x8d, 0xcf, 0x5a, 0x33, 0x5c, 0x18, 0xc5, 0xf4,
        0xe7, 0xfe, 0xee, 0xd3, 0x12, 0x62, 0xad, 0x61, 0x78, 0x9a, 0x03, 0xb0,
        0xaf, 0xab, 0x91, 0x57, 0x46, 0xbf, 0x18, 0xc6, 0xbc, 0x0c, 0x6b, 0x55,
        0xcd, 0xda, 0xc4, 0xcc, 0x98, 0x46, 0x91, 0x99, 0xbc, 0xa3, 0xca, 0x6c,
        0x86, 0xa6, 0x1c, 0x8f, 0xca, 0xf8, 0xf6, 0x8a, 0x00, 0x8e, 0x05, 0xd7,
        0x13, 0x43, 0xe2, 0xf2, 0x1a, 0x13, 0xf3, 0x50, 0x13, 0xa4, 0xf2, 0x4e,
        0x41, 0xb1, 0x36, 0x78, 0x55, 0x4c, 0x5e, 0x27, 0xc5, 0xc0, 0x4b, 0xd8,
        0x93, 0xaa, 0x7e, 0xf0, 0x90, 0x08, 0x10, 0x26, 0x72, 0x6d, 0xb9, 0x21,
        0xae, 0x4d, 0x01, 0x4b, 0x55, 0x1d, 0xe7, 0x1e, 0x5e, 0x31, 0x6e, 0x62,
        0xd1, 0x33, 0x26, 0xcb, 0xdb, 0xfe, 0x72, 0x98, 0xc8, 0x06, 0x1c, 0x12,
        0xdf, 0xfc, 0x74, 0xe5, 0x7a, 0x6f, 0xf5, 0xa3, 0x63, 0x08, 0xe3, 0x02,
        0x68, 0x4d, 0x7c, 0x70, 0x05, 0xec, 0x95, 0x7e, 0x24, 0xa4, 0xbc, 0x4c,
        0xcd, 0x39, 0x14, 0xb5, 0x2a, 0x8f, 0xc1, 0xe3, 0x4e, 0xfa, 0xf8, 0x70,
        0x50, 0x8f, 0xd5, 0x8e, 0xc7, 0xb5, 0x32, 0x89, 0x4d, 0xbb, 0x6a, 0xc1,
        0xc1, 0xa2, 0x42, 0x57, 0x57, 0xbd, 0x2a, 0xdc, 0xa6, 0xfd, 0xc8, 0x86,
        0x44, 0x6a, 0x03, 0x5d, 0x4d, 0x28, 0xe1, 0xde, 0xb4, 0xa9, 0xa5, 0x03,
        0x61, 0x7a, 0x5f, 0xb1, 0x09, 0x17, 0x2b, 0x9c, 0xa2, 0x54, 0x28, 0xad,
        0x34, 0xc9, 0x5f, 0x6c, 0x9f, 0xb8, 0xd2, 0xa9, 0x78, 0xa7, 0xaa, 0xb3,
        0x11, 0x2f, 0x65, 0x9b, 0x4e, 0x67, 0x0c, 0xcc, 0x20, 0x36, 0xbf, 0x26,
        0x2b, 0x4e, 0xc0, 0xd4, 0xbd, 0x22, 0x64, 0xc4, 0x1c, 0x56, 0x69, 0xdb,
        0x5f, 0x89, 0xe1, 0x75, 0x68, 0x8d, 0x0e, 0xab, 0x1c, 0x10, 0x1a, 0xc0,
        0x12, 0x5d, 0x6f, 0xbd, 0x09, 0xbb, 0x47, 0xcb, 0xe7, 0x34, 0xef, 0x56,
        0xab, 0xea, 0xc3, 0xe9, 0x7f, 0x9a, 0x3d, 0xe9, 0x2d, 0x14, 0x61, 0x25,
        0x37, 0x5c, 0x3b, 0x4b, 0xaf, 0x5a, 0x4b, 0xc8, 0x99, 0x1a, 0x32, 0x8f,
        0x54, 0x07, 0xd3, 0x57, 0x8a, 0x3d, 0x2a, 0xf7, 0x9e, 0x7e, 0x92, 0x2a,
        0x50, 0xe9, 0xd8, 0xdb, 0xd6, 0x03, 0xd3, 0x8e, 0x54, 0x32, 0xce, 0x87,
        0x93, 0x92, 0xe7, 0x75, 0xe1, 0x6b, 0x78, 0x1a, 0x85, 0xc2, 0x46, 0xa1,
        0x31, 0xbb, 0xc7, 0xb9, 0x1d, 0xd1, 0x71, 0xe0, 0xe2, 0x9b, 0x9c, 0x0d,
        0xa3, 0xcf, 0x93, 0x4d, 0x87, 0x7b, 0x65, 0xd9, 0xda, 0x4c, 0xd9, 0x6a,
        0xa6, 0x36, 0xc2, 0xc7, 0xe3, 0x33, 0xe2, 0xc3, 0x83, 0xd1, 0x72, 0x54,
        0x30, 0x81, 0x5e, 0x34, 0x2c, 0x61, 0xee, 0xf4, 0x48, 0x97, 0xb6, 0xaa,
        0x47, 0x6a, 0x05, 0x09, 0xd8, 0x4d, 0x90, 0xaf, 0xa8, 0x4e, 0x82, 0xe4,
        0x8e, 0xb5, 0xe2, 0x65, 0x86, 0x67, 0xe9, 0x5b, 0x4b, 0x9a, 0x68, 0x08,
        0x30, 0xf6, 0x25, 0x8b, 0x20, 0xda, 0x26, 0x6f, 0xbd, 0x0d, 0xa5, 0xd8,
        0x6a, 0x7b, 0x01, 0x2f, 0xab, 0x7b, 0xb5, 0xfe, 0x62, 0x37, 0x2d, 0x94,
        0x43, 0x2f, 0x4d, 0x16, 0x01, 0x00, 0x01, 0x00,
};

// 2048 bit RSA signature.
const uint8_t kSignature2048[ANDROID_PUBKEY_MODULUS_SIZE] = {
        0x3a, 0x11, 0x84, 0x40, 0xc1, 0x2f, 0x13, 0x8c, 0xde, 0xb0, 0xc3, 0x89,
        0x8a, 0x63, 0xb2, 0x50, 0x93, 0x58, 0xc0, 0x0c, 0xb7, 0x08, 0xe7, 0x6c,
        0x52, 0x87, 0x4e, 0x78, 0x89, 0xa3, 0x9a, 0x47, 0xeb, 0x11, 0x57, 0xbc,
        0xb3, 0x97, 0xf8, 0x34, 0xf1, 0xf7, 0xbf, 0x3a, 0xfa, 0x1c, 0x6b, 0xdc,
        0xd1, 0x02, 0xde, 0x9a, 0x0d, 0x72, 0xe7, 0x19, 0x63, 0x81, 0x46, 0x68,
        0x1e, 0x63, 0x64, 0xc6, 0x59, 0xe7, 0x7c, 0x39, 0xed, 0x32, 0xd2, 0xd1,
        0xd5, 0x1f, 0x13, 0x9b, 0x52, 0xdf, 0x34, 0xa3, 0xc0, 0xc4, 0x9a, 0x63,
        0x9b, 0x9c, 0xbe, 0x22, 0xc8, 0xd8, 0x14, 0x2f, 0x4c, 0x78, 0x36, 0xdb,
        0x16, 0x41, 0x67, 0xc1, 0x21, 0x8a, 0x73, 0xb2, 0xe5, 0xb0, 0xd3, 0x80,
        0x91, 0x7a, 0xbf, 0xf9, 0x59, 0x4a, 0x4d, 0x78, 0x45, 0x44, 0xa1, 0x52,
        0x86, 0x29, 0x48, 0x4d, 0xf0, 0x5d, 0xf2, 0x55, 0xa7, 0xcd, 0xc5, 0x2b,
        0x7b, 0xe0, 0xb1, 0xf6, 0x2a, 0xd5, 0x61, 0xba, 0x1e, 0x1e, 0x3a, 0xf0,
        0x55, 0xbc, 0x8c, 0x44, 0x41, 0xfc, 0xb8, 0x8c, 0x76, 0xbf, 0x80, 0x58,
        0x82, 0x35, 0x4b, 0x0c, 0xfd, 0xef, 0xd5, 0x70, 0xd1, 0x64, 0xcb, 0x46,
        0x58, 0x37, 0xbc, 0xa9, 0x7d, 0xd4, 0x70, 0xac, 0xce, 0xec, 0xca, 0x48,
        0xcb, 0x0a, 0x40, 0x77, 0x04, 0x59, 0xca, 0x9c, 0x7d, 0x1a, 0x0b, 0xf0,
        0xb5, 0xdd, 0xde, 0x71, 0x18, 0xb8, 0xef, 0x90, 0x2a, 0x09, 0x42, 0x39,
        0x74, 0xff, 0x45, 0xa1, 0x39, 0x17, 0x50, 0x89, 0xa6, 0x5f, 0xbc, 0x9c,
        0x0c, 0x9b, 0x47, 0x25, 0x79, 0x3e, 0xe3, 0xaa, 0xaf, 0xbe, 0x73, 0x6b,
        0xcb, 0xe7, 0x35, 0xc1, 0x27, 0x09, 0xcd, 0xeb, 0xd7, 0xcf, 0x63, 0x83,
        0x64, 0x8c, 0x45, 0x1c, 0x1d, 0x58, 0xcc, 0xd2, 0xf8, 0x2b, 0x4c, 0x4e,
        0x14, 0x89, 0x2d, 0x70,
};

struct AndroidPubkeyTest : public ::testing::Test {
    void SetUp() override {
        RSA* new_key = nullptr;
        android_pubkey_decode(kKey2048, sizeof(kKey2048), &new_key);
        key_.reset(new_key);
    }

    std::unique_ptr<RSA, void (*)(RSA*)> key_ = {nullptr, RSA_free};
};

TEST_F(AndroidPubkeyTest, Decode) {
    // Make sure the decoded key successfully verifies a valid signature.
    EXPECT_TRUE(RSA_verify(NID_sha256, kDigest, sizeof(kDigest), kSignature2048,
                           sizeof(kSignature2048), key_.get()));
}

TEST_F(AndroidPubkeyTest, Encode) {
    //uint8_t key_data[ANDROID_PUBKEY_ENCODED_SIZE];
    uint8_t key_data[ANDROID_PUBKEY_ENCODED_SIZE];
    ASSERT_TRUE(android_pubkey_encode(key_.get(), key_data, sizeof(key_data)));
    ASSERT_EQ(0, memcmp(kKey2048, key_data, sizeof(kKey2048)));
}

}  // namespace base
}  // namespace android
