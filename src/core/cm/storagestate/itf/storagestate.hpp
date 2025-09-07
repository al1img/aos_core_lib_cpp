/*
 * Copyright (C) 2025 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_CORE_CM_STORAGESTATE_ITF_STORAGESTATE_HPP_
#define AOS_CORE_CM_STORAGESTATE_ITF_STORAGESTATE_HPP_

#include <core/common/types/types.hpp>

namespace aos::cm::storagestate {

/** @addtogroup cm Communication Manager
 *  @{
 */

/**
 * Setup storage state instance params.
 */
struct SetupParams {
    uid_t  mUID {};
    gid_t  mGID {};
    size_t mStateQuota {};
    size_t mStorageQuota {};
};

/**
 * Interface to manage storage and state partitions.
 */
class StorageStateItf : public NonCopyable {
public:
    /**
     * Setups storage state instance.
     *
     * @param instanceIdent instance ident.
     * @param setupParams setup parameters.
     * @param storagePath[out] storage path.
     * @param statePath[out] state path.
     * @return Error.
     */
    virtual Error Setup(
        const InstanceIdent& instanceIdent, const SetupParams& setupParams, String& storagePath, String& statePath)
        = 0;

    /**
     * Clean-ups storage state instance.
     *
     * @param instanceIdent instance ident.
     * @return Error.
     */
    virtual Error Cleanup(const InstanceIdent& instanceIdent) = 0;

    /**
     * Removes storage state instance.
     *
     * @param instanceIdent instance ident.
     * @return Error.
     */
    virtual Error Remove(const InstanceIdent& instanceIdent) = 0;

    /**
     * Returns instance's checksum.
     *
     * @param instanceIdent instance ident.
     * @param checkSum[out] checksum.
     * @return Error
     */
    virtual Error GetInstanceCheckSum(const InstanceIdent& instanceIdent, String& checkSum) = 0;

    /**
     * Updates storage state with new state.
     *
     * @param instanceIdent instance ident.
     * @param state new state.
     * @param checkSum state checksum.
     * @return Error.
     */
    virtual Error UpdateState(const InstanceIdent& instanceIdent, const Array<uint8_t>& state, const String& checkSum)
        = 0;

    /**
     * Accepts state from storage.
     *
     * @param instanceIdent instance ident.
     * @param checkSum state checksum.
     * @param result state acceptance result.
     * @param reason reason for state acceptance/rejection.
     * @return Error.
     */
    virtual Error AcceptState(
        const InstanceIdent& instanceIdent, const String& checkSum, StateResult result, const String& reason)
        = 0;

    /**
     * Destructor.
     */
    virtual ~StorageStateItf() = default;
};

/** @}*/

} // namespace aos::cm::storagestate

#endif
