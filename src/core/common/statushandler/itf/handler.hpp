/*
 * Copyright (C) 2026 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_CORE_COMMON_STATUSHANDLER_ITF_HANDLER_HPP_
#define AOS_CORE_COMMON_STATUSHANDLER_ITF_HANDLER_HPP_

#include <core/common/types/instance.hpp>

namespace aos::statushandler {

/**
 * Instance status handler interface.
 */
class HandlerItf {
public:
    /**
     * Destructor.
     */
    virtual ~HandlerItf() = default;

    /**
     * Sets statuses of all running instances.
     *
     * @param statuses instances statuses.
     * @return Error.
     */
    virtual Error SetAllInstancesStatuses(const Array<InstanceStatus>& statuses) = 0;

    /**
     * Sets status of a single instance.
     *
     * @param status instance status.
     * @return Error.
     */
    virtual Error SetInstanceStatus(const InstanceStatus& status) = 0;
};

} // namespace aos::statushandler

#endif
