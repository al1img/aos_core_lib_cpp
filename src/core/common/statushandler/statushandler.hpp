/*
 * Copyright (C) 2026 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_CORE_COMMON_STATUSHANDLER_STATUSHANDLER_HPP_
#define AOS_CORE_COMMON_STATUSHANDLER_STATUSHANDLER_HPP_

#include "itf/handler.hpp"
#include "itf/provider.hpp"

namespace aos::statushandler {

/**
 * Instance status handler.
 */
class StatusHandler : public HandlerItf, public ProviderItf {
public:
    /**
     * Sets statuses of all running instances.
     *
     * @param statuses instances statuses.
     * @return Error.
     */
    Error SetAllInstancesStatuses(const Array<InstanceStatus>& statuses) override;

    /**
     * Sets status of a single instance.
     *
     * @param status instance status.
     * @return Error.
     */
    Error SetInstanceStatus(const InstanceStatus& status) override;

    /**
     * Returns current statuses of running instances.
     *
     * @param[out] statuses instances statuses.
     * @return Error.
     */
    Error GetInstancesStatuses(Array<InstanceStatus>& statuses) override;

    /**
     * Subscribes status notifications.
     *
     * @param listener status listener.
     * @return Error.
     */
    Error SubscribeListener(ListenerItf& listener) override;

    /**
     * Unsubscribes from status notifications.
     *
     * @param listener status listener.
     * @return Error.
     */
    Error UnsubscribeListener(ListenerItf& listener) override;

private:
    constexpr static auto cMaxNumListeners = 2;

    void NotifyListeners(const InstanceStatus& status);

    Mutex                                         mMutex;
    StaticArray<InstanceStatus, cMaxNumInstances> mStatuses;
    StaticArray<ListenerItf*, cMaxNumListeners>   mListeners;
};

} // namespace aos::statushandler

#endif
