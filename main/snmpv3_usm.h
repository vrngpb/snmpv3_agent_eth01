#pragma once
#include <stdint.h>

/*
 * SNMPv3 User Security Model (USM) — RFC 3414
 *
 * Security level: authPriv  (SHA-1 authentication + AES-128 encryption)
 *
 * Credentials are set at build time via cmake -D... variables.
 * Change defaults before deploying to production.
 */

#ifndef SNMPV3_USERNAME
#define SNMPV3_USERNAME         "snmpv3admin"
#endif

/* Minimum 8 characters required by RFC 3414 */
#ifndef SNMPV3_AUTH_PASSPHRASE
#define SNMPV3_AUTH_PASSPHRASE  "authpassphrase01"
#endif

#ifndef SNMPV3_PRIV_PASSPHRASE
#define SNMPV3_PRIV_PASSPHRASE  "privpassphrase01"
#endif

/*
 * Initializes the SNMPv3 USM:
 *   - builds engine ID from MAC (RFC 3411 enterprise-MAC format)
 *   - loads / increments engine boots counter from NVS
 *   - derives localized SHA-1 auth key and AES-128 priv key
 *   - starts the 1-second engine time tick timer
 *
 * Must be called before snmp_init().
 */
void snmpv3_usm_init(const uint8_t *mac);
