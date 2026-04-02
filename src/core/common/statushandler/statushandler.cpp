/*
 * Copyright (C) 2026 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core/common/tools/logger.hpp>

#include "statushandler.hpp"

namespace aos::statushandler {

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error StatusHandler::SetAllInstancesStatuses(const Array<InstanceStatus>& statuses)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Set all instances statuses" << Log::Field("count", statuses.Size());

    for (const auto& status : statuses) {
        LOG_DBG() << "Instance status" << Log::Field("instance", static_cast<const InstanceIdent&>(status))
                  << Log::Field("version", status.mVersion) << Log::Field("node", status.mNodeID)
                  << Log::Field("runtime", status.mRuntimeID) << Log::Field("manifest", status.mManifestDigest)
                  << Log::Field("state", status.mState);

        auto it = mStatuses.FindIf([&](const auto& existingStatus) {
            return existingStatus.mItemID == status.mItemID && existingStatus.mSubjectID == status.mSubjectID
                && existingStatus.mInstance == status.mInstance && existingStatus.mType == status.mType;
        });

        if (it == mStatuses.end() || *it != status) {
            NotifyListeners(status);
        }
    }

    if (auto err = mStatuses.Assign(statuses); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error StatusHandler::SetInstanceStatus(const InstanceStatus& status)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Set instance status" << Log::Field("instance", static_cast<const InstanceIdent&>(status))
              << Log::Field("version", status.mVersion) << Log::Field("node", status.mNodeID)
              << Log::Field("runtime", status.mRuntimeID) << Log::Field("manifest", status.mManifestDigest)
              << Log::Field("state", status.mState);

    auto it = mStatuses.FindIf([&](const auto& existingStatus) {
        return existingStatus.mItemID == status.mItemID && existingStatus.mSubjectID == status.mSubjectID
            && existingStatus.mInstance == status.mInstance && existingStatus.mType == status.mType;
    });
    if (it == mStatuses.end()) {
        return ErrorEnum::eNotFound;
    }

    if (*it == status) {
        return ErrorEnum::eNone;
    }

    *it = status;

    NotifyListeners(status);

    return ErrorEnum::eNone;
}

Error StatusHandler::GetInstancesStatuses(Array<InstanceStatus>& statuses)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Get instances statuses" << Log::Field("count", mStatuses.Size());

    if (auto err = statuses.Assign(mStatuses); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error StatusHandler::SubscribeListener(ListenerItf& listener)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Subscribe listener";

    if (mListeners.Contains(&listener)) {
        return ErrorEnum::eAlreadyExist;
    }

    if (auto err = mListeners.PushBack(&listener); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error StatusHandler::UnsubscribeListener(ListenerItf& listener)
{
    LockGuard lock {mMutex};

    LOG_DBG() << "Unsubscribe listener";

    if (mListeners.Remove(&listener) == 0) {
        return ErrorEnum::eNotFound;
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

void StatusHandler::NotifyListeners(const InstanceStatus& status)
{
    for (auto* listener : mListeners) {
        listener->OnInstanceStatusChanged(status);
    }
}

} // namespace aos::statushandler
