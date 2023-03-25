/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_TOOLS_CONFIG_HPP_
#define AOS_TOOLS_CONFIG_HPP_

/**
 * Log levels.
 *
 */
#define AOS_CONFIG_LOG_LEVEL_DISABLE 0
#define AOS_CONFIG_LOG_LEVEL_ERROR   1
#define AOS_CONFIG_LOG_LEVEL_WARNING 2
#define AOS_CONFIG_LOG_LEVEL_INFO    3
#define AOS_CONFIG_LOG_LEVEL_DEBUG   4

/**
 * Configures log level.
 *
 */
#ifndef AOS_CONFIG_LOG_LEVEL
#define AOS_CONFIG_LOG_LEVEL AOS_CONFIG_LOG_LEVEL_DEBUG
#endif

/**
 * Max log line size.
 *
 */
#ifndef AOS_CONFIG_LOG_LINE_LEN
#define AOS_CONFIG_LOG_LINE_LEN 120
#endif

/**
 * Configures max thread task size.
 */
#ifndef AOS_CONFIG_THREAD_DEFAULT_MAX_FUNCTION_SIZE
#define AOS_CONFIG_THREAD_DEFAULT_MAX_FUNCTION_SIZE 256
#endif

/**
 * Configures default thread stack size.
 *
 * Use minimal stack size PTHREAD_STACK_MIN + 2k for storing thread functor.
 */
#ifndef AOS_CONFIG_THREAD_DEFAULT_STACK_SIZE
#define AOS_CONFIG_THREAD_DEFAULT_STACK_SIZE (16384 + 2048)
#endif

/**
 * Configures thread stack alignment.
 */
#ifndef AOS_CONFIG_THREAD_STACK_ALIGN
#define AOS_CONFIG_THREAD_STACK_ALIGN sizeof(int)
#endif

/**
 * Configures default thread pool queue size.
 */
#ifndef AOS_CONFIG_THREAD_POOL_DEFAULT_QUEUE_SIZE
#define AOS_CONFIG_THREAD_POOL_DEFAULT_QUEUE_SIZE 1024
#endif

/**
 * Enables Aos custom new operators.
 */
#ifndef AOS_CONFIG_NEW_USE_AOS
#define AOS_CONFIG_NEW_USE_AOS 0
#endif

#endif
