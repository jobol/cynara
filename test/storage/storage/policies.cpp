/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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
 *    limitations under the License.
 */
/**
 * @file        test/storage/storage/policies.cpp
 * @author      Aleksander Zdyb <a.zdyb@samsung.com>
 * @version     1.0
 * @brief       Tests of policies in Storage
 */

#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <exceptions/BucketNotExistsException.h>
#include <storage/Storage.h>
#include <storage/StorageBackend.h>
#include <types/pointers.h>
#include <types/PolicyCollection.h>
#include <types/PolicyKey.h>
#include <types/PolicyResult.h>
#include <types/PolicyType.h>

#include "../../helpers.h"
#include "fakestoragebackend.h"

using namespace Cynara;

TEST(storage, deletePolicies) {
    FakeStorageBackend backend;
    Cynara::Storage storage(backend);

    auto pk1 = Helpers::generatePolicyKey("1");
    auto pk2 = Helpers::generatePolicyKey("2");
    PolicyBucketId bucketId1 = "bucket-1";
    PolicyBucketId bucketId2 = "bucket-2";

    EXPECT_CALL(backend, deletePolicy(bucketId1, pk1));
    EXPECT_CALL(backend, deletePolicy(bucketId1, pk2));
    EXPECT_CALL(backend, deletePolicy(bucketId2, pk1));

    typedef std::pair<PolicyBucketId, std::vector<PolicyKey>> BucketPoliciesPair;

    storage.deletePolicies({
        BucketPoliciesPair(bucketId1, { pk1, pk2 }),
        BucketPoliciesPair(bucketId2, { pk1 }),
    });
}

// TODO: isn't it the same test as storage.deleteBucket?
TEST(storage, deleteBucketWithLinkedPolicies) {
    FakeStorageBackend backend;
    Cynara::Storage storage(backend);

    PolicyBucketId bucketId = "test-bucket";

    EXPECT_CALL(backend, deleteLinking(bucketId));
    EXPECT_CALL(backend, deleteBucket(bucketId));

    storage.deleteBucket(bucketId);
}

TEST(storage, insertPolicies) {
    using ::testing::Pointee;
    using ::testing::Return;
    using PredefinedPolicyType::ALLOW;
    using PredefinedPolicyType::BUCKET;
    using PredefinedPolicyType::DENY;

    FakeStorageBackend backend;
    Storage storage(backend);

    std::vector<PolicyBucketId> testBuckets = {
        PolicyBucketId("test-bucket-1"),
        PolicyBucketId("test-bucket-2"),
    };

    PolicyResult defaultPolicy(PredefinedPolicyType::DENY);

    typedef std::pair<PolicyBucketId, std::vector<Policy>> BucketPolicyPair;

    auto createPolicy = [] (const std::string &keySuffix, const PolicyResult &result) -> Policy {
        return Policy(Helpers::generatePolicyKey(keySuffix), result);
    };

    std::map<PolicyBucketId, std::vector<Policy>> policiesToInsert = {
        BucketPolicyPair(testBuckets[0], {
            createPolicy("1", ALLOW),
            createPolicy("2", DENY),
            createPolicy("3", DENY)
        }),
        BucketPolicyPair(testBuckets[1], {
            createPolicy("4", { BUCKET, testBuckets[0] }),
            createPolicy("5", PredefinedPolicyType::ALLOW)
        })
    };

    for (const auto &bucket: testBuckets) {
        EXPECT_CALL(backend, hasBucket(bucket)).WillOnce(Return(false));
        EXPECT_CALL(backend, createBucket(bucket, defaultPolicy)).WillOnce(Return());

        storage.addOrUpdateBucket(bucket, defaultPolicy);
    }

    EXPECT_CALL(backend, hasBucket(testBuckets[0])).WillRepeatedly(Return(true));
    EXPECT_CALL(backend, hasBucket(testBuckets[1])).WillOnce(Return(true));

    for (const auto &group : policiesToInsert) {
        const auto &bucketId = group.first;
        const auto &policies = group.second;

        for (const auto &policy : policies) {
            EXPECT_CALL(backend, insertPolicy(bucketId, Pointee(policy)));
        }
    }

    storage.insertPolicies(policiesToInsert);
}

TEST(storage, insertPointingToNonexistentBucket) {
    using ::testing::Pointee;
    using ::testing::Return;
    FakeStorageBackend backend;
    Storage storage(backend);

    PolicyBucketId testBucketId = "test-bucket-1";
    PolicyBucketId nonexistentBucketId = "nonexistent";

    PolicyResult defaultPolicy(PredefinedPolicyType::DENY);

    typedef std::pair<PolicyBucketId, std::vector<Policy>> BucketPolicyPair;

    auto createPolicy = [] (const std::string &keySuffix, const PolicyResult &result) -> Policy {
        return Policy(Helpers::generatePolicyKey(keySuffix), result);
    };

    std::map<PolicyBucketId, std::vector<Policy>> policiesToInsert = {
        BucketPolicyPair(testBucketId, {
            createPolicy("1", { PredefinedPolicyType::DENY }),
            createPolicy("2", { PredefinedPolicyType::BUCKET, nonexistentBucketId }),
        }),
    };

    EXPECT_CALL(backend, hasBucket(testBucketId)).WillOnce(Return(false));
    EXPECT_CALL(backend, createBucket(testBucketId, defaultPolicy)).WillOnce(Return());

    storage.addOrUpdateBucket(testBucketId, defaultPolicy);

    EXPECT_CALL(backend, hasBucket(testBucketId)).WillOnce(Return(true));
    EXPECT_CALL(backend, hasBucket(nonexistentBucketId)).WillOnce(Return(false));

    ASSERT_THROW(storage.insertPolicies(policiesToInsert), BucketNotExistsException);
}

TEST(storage, listPolicies) {
    using ::testing::Return;
    using PredefinedPolicyType::DENY;

    FakeStorageBackend backend;
    Storage storage(backend);

    PolicyBucketId testBucket("test-bucket");
    PolicyResult defaultPolicy(PredefinedPolicyType::DENY);

    EXPECT_CALL(backend, hasBucket(testBucket)).WillOnce(Return(false));
    EXPECT_CALL(backend, createBucket(testBucket, defaultPolicy)).WillOnce(Return());
    storage.addOrUpdateBucket(testBucket, defaultPolicy);

    PolicyKey filter = Helpers::generatePolicyKey("1");
    EXPECT_CALL(backend, listPolicies(testBucket, filter))
    .WillOnce(Return(PolicyBucket::Policies()));

    storage.listPolicies(testBucket, filter);
}

TEST(storage, listPoliciesFromNonexistentBucket) {
    using ::testing::Return;
    using PredefinedPolicyType::DENY;

    FakeStorageBackend backend;
    Storage storage(backend);

    PolicyBucketId testBucket("test-bucket");
    PolicyResult defaultPolicy(PredefinedPolicyType::DENY);
    PolicyKey filter = Helpers::generatePolicyKey("1");
    EXPECT_CALL(backend, listPolicies(testBucket, filter))
    .WillOnce(Return(PolicyBucket::Policies()));

    storage.listPolicies(testBucket, filter);
}
