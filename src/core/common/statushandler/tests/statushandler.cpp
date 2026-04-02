/*
 * Copyright (C) 2026 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <core/common/statushandler/statushandler.hpp>
#include <core/common/tests/utils/log.hpp>

using namespace testing;

namespace aos::statushandler {

/***********************************************************************************************************************
 * Suite
 **********************************************************************************************************************/

class StatusHandlerTest : public Test {
protected:
    void SetUp() override { tests::utils::InitLog(); }

    StatusHandler mStatusHandler;
};

/***********************************************************************************************************************
 * Tests
 **********************************************************************************************************************/

} // namespace aos::statushandler
