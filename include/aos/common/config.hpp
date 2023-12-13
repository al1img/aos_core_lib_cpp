/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_COMMON_CONFIG_HPP_
#define AOS_COMMON_CONFIG_HPP_

/**
 * Service/layer description len.
 */
#ifndef AOS_CONFIG_TYPES_PROVIDER_ID_LEN
#define AOS_CONFIG_TYPES_PROVIDER_ID_LEN 40
#endif

/**
 * Service ID len.
 */
#ifndef AOS_CONFIG_TYPES_SERVICE_ID_LEN
#define AOS_CONFIG_TYPES_SERVICE_ID_LEN 40
#endif

/**
 * Subject ID len.
 */
#ifndef AOS_CONFIG_TYPES_SUBJECT_ID_LEN
#define AOS_CONFIG_TYPES_SUBJECT_ID_LEN 40
#endif

/**
 * Layer ID len.
 */
#ifndef AOS_CONFIG_TYPES_LAYER_ID_LEN
#define AOS_CONFIG_TYPES_LAYER_ID_LEN 40
#endif

/**
 * Layer digest len.
 */
#ifndef AOS_CONFIG_TYPES_LAYER_DIGEST_LEN
#define AOS_CONFIG_TYPES_LAYER_DIGEST_LEN 128
#endif

/**
 * Instance ID len.
 */
#ifndef AOS_CONFIG_TYPES_INSTANCE_ID_LEN
#define AOS_CONFIG_TYPES_INSTANCE_ID_LEN 40
#endif

/**
 * URL len.
 */
#ifndef AOS_CONFIG_TYPES_URL_LEN
#define AOS_CONFIG_TYPES_URL_LEN 256
#endif

/**
 * Vendor version len.
 */
#ifndef AOS_CONFIG_TYPES_VENDOR_VERSION_LEN
#define AOS_CONFIG_TYPES_VENDOR_VERSION_LEN 20
#endif

/**
 * Service/layer description len.
 */
#ifndef AOS_CONFIG_TYPES_DESCRIPTION_LEN
#define AOS_CONFIG_TYPES_DESCRIPTION_LEN 200
#endif

/**
 * Max number of services.
 */
#ifndef AOS_CONFIG_TYPES_MAX_NUM_SERVICES
#define AOS_CONFIG_TYPES_MAX_NUM_SERVICES 16
#endif

/**
 * Max number of layers.
 */
#ifndef AOS_CONFIG_TYPES_MAX_NUM_LAYERS
#define AOS_CONFIG_TYPES_MAX_NUM_LAYERS 16
#endif

/**
 * Max number of instances.
 */
#ifndef AOS_CONFIG_TYPES_MAX_NUM_INSTANCES
#define AOS_CONFIG_TYPES_MAX_NUM_INSTANCES 16
#endif

/**
 * Error message len.
 */
#ifndef AOS_CONFIG_TYPES_ERROR_MESSAGE_LEN
#define AOS_CONFIG_TYPES_ERROR_MESSAGE_LEN 256
#endif

/**
 * File chunk size.
 */
#ifndef AOS_CONFIG_TYPES_FILE_CHUNK_SIZE
#define AOS_CONFIG_TYPES_FILE_CHUNK_SIZE 1024
#endif

/**
 * Max number of partitions.
 */
#ifndef AOS_CONFIG_TYPES_MAX_NUM_PARTITIONS
#define AOS_CONFIG_TYPES_MAX_NUM_PARTITIONS 4
#endif

/**
 * Max number of partition types.
 */
#ifndef AOS_CONFIG_TYPES_MAX_NUM_PARTITION_TYPES
#define AOS_CONFIG_TYPES_MAX_NUM_PARTITION_TYPES 4
#endif

/**
 * Partition name len.
 */
#ifndef AOS_CONFIG_TYPES_PARTITION_NAME_LEN
#define AOS_CONFIG_TYPES_PARTITION_NAME_LEN 64
#endif

/**
 * Partition types len.
 */
#ifndef AOS_CONFIG_TYPES_PARTITION_TYPES_LEN
#define AOS_CONFIG_TYPES_PARTITION_TYPES_LEN 32
#endif

/**
 * Node ID len.
 */
#ifndef AOS_CONFIG_TYPES_NODE_ID_LEN
#define AOS_CONFIG_TYPES_NODE_ID_LEN 64
#endif

/**
 * Monitoring period send.
 */
#ifndef AOS_CONFIG_MONITORING_SEND_PERIOD_SEC
#define AOS_CONFIG_MONITORING_SEND_PERIOD_SEC 60
#endif

/**
 * Monitoring period poll.
 */
#ifndef AOS_CONFIG_MONITORING_POLL_PERIOD_SEC
#define AOS_CONFIG_MONITORING_POLL_PERIOD_SEC 10
#endif

/**
 * Certificate public key size(in bytes).
 */
#ifndef AOS_CONFIG_CRYPTO_CERT_PUB_KEY_SIZE
#define AOS_CONFIG_CRYPTO_CERT_PUB_KEY_SIZE 2048
#endif

/**
 * Certificate issuer size(in bytes).
 */
#ifndef AOS_CONFIG_CRYPTO_CERT_ISSUER_SIZE
#define AOS_CONFIG_CRYPTO_CERT_ISSUER_SIZE 256
#endif

/**
 * Max length of a DNS name.
 */
#ifndef AOS_CONFIG_CRYPTO_DNS_NAME_LEN
#define AOS_CONFIG_CRYPTO_DNS_NAME_LEN 42
#endif

/**
 * Max number of alternative DNS names for a module.
 */
#ifndef AOS_CONFIG_CRYPTO_ALT_DNS_NAMES_MAX_COUNT
#define AOS_CONFIG_CRYPTO_ALT_DNS_NAMES_MAX_COUNT 4
#endif

/**
 * Raw certificate request size.
 */
#ifndef AOS_CONFIG_CRYPTO_EXTRA_EXTENSIONS_COUNT
#define AOS_CONFIG_CRYPTO_EXTRA_EXTENSIONS_COUNT 10
#endif

/**
 * Maximum length of numeric string representing ASN.1 Object Identifier.
 */
#ifndef AOS_CONFIG_CRYPTO_ASN1_OBJECT_ID_LEN
#define AOS_CONFIG_CRYPTO_ASN1_OBJECT_ID_LEN 24
#endif

/**
 * Maximum size of a certificate ASN.1 Extension Value.
 */
#ifndef AOS_CONFIG_CRYPTO_ASN1_EXTENSION_VALUE_SIZE
#define AOS_CONFIG_CRYPTO_ASN1_EXTENSION_VALUE_SIZE 16
#endif

/**
 * Maximum certificate key id size(in bytes).
 */
#ifndef AOS_CONFIG_CRYPTO_CERT_KEY_ID_SIZE
#define AOS_CONFIG_CRYPTO_CERT_KEY_ID_SIZE 24
#endif

/**
 * Maximum size of a PEM certificate.
 */
#ifndef AOS_CONFIG_CRYPTO_PEM_CERT_SIZE
#define AOS_CONFIG_CRYPTO_PEM_CERT_SIZE 4096
#endif

/**
 * Serial number size(in bytes).
 */
#ifndef AOS_CONFIG_CRYPTO_SERIAL_NUM_SIZE
#define AOS_CONFIG_CRYPTO_SERIAL_NUM_SIZE 20
#endif

/**
 * Length of serial number in string representation.
 */
#ifndef AOS_CONFIG_CRYPTO_SERIAL_NUM_STR_LEN
#define AOS_CONFIG_CRYPTO_SERIAL_NUM_STR_LEN 40
#endif

#endif
