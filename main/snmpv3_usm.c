#include "snmpv3_usm.h"

#include "lwip/apps/snmpv3.h"
#include "lwip/err.h"
#include "lwip/def.h"
#include "nvs.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>
#include <stdint.h>

#if LWIP_SNMP && LWIP_SNMP_V3

#define TAG              "SNMPV3_USM"
#define NVS_NAMESPACE    "storage"
#define NVS_KEY_BOOTS    "snmp_boots"

/* Enterprise OID used to build engine ID (must match OID base in main.c) */
#define SNMP_ENTERPRISE  9999U

/* ------------------------------------------------------------------ */
/* User table                                                          */
/* ------------------------------------------------------------------ */

struct user_entry {
    char               username[32];
    snmpv3_auth_algo_t auth_algo;
    u8_t               auth_key[20];   /* SHA-1 localized key (20 bytes) */
    snmpv3_priv_algo_t priv_algo;
    u8_t               priv_key[20];   /* SHA-1 derived; AES uses first 16 */
};

static struct user_entry s_users[] = {
    {
        .username  = SNMPV3_USERNAME,
        .auth_algo = SNMP_V3_AUTH_ALGO_SHA,
        .priv_algo = SNMP_V3_PRIV_ALGO_AES,
    }
};

static struct user_entry *find_user(const char *username)
{
    for (size_t i = 0; i < LWIP_ARRAYSIZE(s_users); i++) {
        if (strcmp(username, s_users[i].username) == 0) {
            return &s_users[i];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Engine state                                                        */
/* ------------------------------------------------------------------ */

static char     s_engine_id[20];
static u8_t     s_engine_id_len = 0;
static uint32_t s_engine_boots  = 0;
static volatile uint32_t s_engine_time = 0;

/* ------------------------------------------------------------------ */
/* 1-second periodic timer — increments engine time                   */
/* ------------------------------------------------------------------ */

static void engine_time_tick(void *arg)
{
    (void)arg;
    s_engine_time++;
    /*
     * snmpv3_get_engine_time_internal() (provided by lwIP) is also called
     * by the stack on every inbound message, so a per-tick call here is not
     * required.  Calling it from the esp_timer context would cross task
     * boundaries, so we deliberately omit it.  Overflow at ~68 years is
     * not a practical concern for this device.
     */
}

/* ------------------------------------------------------------------ */
/* Callbacks: engine ID                                               */
/* ------------------------------------------------------------------ */

void snmpv3_get_engine_id(const char **id, u8_t *len)
{
    *id  = s_engine_id;
    *len = s_engine_id_len;
}

err_t snmpv3_set_engine_id(const char *id, u8_t len)
{
    if (len > (u8_t)sizeof(s_engine_id)) {
        return ERR_VAL;
    }
    MEMCPY(s_engine_id, id, len);
    s_engine_id_len = len;
    return ERR_OK;
}

/* ------------------------------------------------------------------ */
/* Callbacks: engine boots (persisted in NVS)                         */
/* ------------------------------------------------------------------ */

u32_t snmpv3_get_engine_boots(void)
{
    return s_engine_boots;
}

void snmpv3_set_engine_boots(u32_t boots)
{
    s_engine_boots = boots;
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u32(h, NVS_KEY_BOOTS, boots);
        nvs_commit(h);
        nvs_close(h);
    }
}

/* ------------------------------------------------------------------ */
/* Callbacks: engine time                                             */
/* ------------------------------------------------------------------ */

u32_t snmpv3_get_engine_time(void)
{
    return s_engine_time;
}

void snmpv3_reset_engine_time(void)
{
    s_engine_time = 0;
}

/* ------------------------------------------------------------------ */
/* Callbacks: user table                                              */
/* ------------------------------------------------------------------ */

u8_t snmpv3_get_amount_of_users(void)
{
    return (u8_t)LWIP_ARRAYSIZE(s_users);
}

err_t snmpv3_get_username(char *username, u8_t index)
{
    if (index >= LWIP_ARRAYSIZE(s_users)) {
        return ERR_VAL;
    }
    MEMCPY(username, s_users[index].username, sizeof(s_users[0].username));
    return ERR_OK;
}

err_t snmpv3_get_user_storagetype(const char *username, snmpv3_user_storagetype_t *type)
{
    if (find_user(username) == NULL) {
        return ERR_VAL;
    }
    *type = SNMP_V3_USER_STORAGETYPE_READONLY;
    return ERR_OK;
}

err_t snmpv3_get_user(const char *username,
                      snmpv3_auth_algo_t *auth_algo, u8_t *auth_key,
                      snmpv3_priv_algo_t *priv_algo, u8_t *priv_key)
{
    /* Empty username = engine discovery probe; allow it without a user match */
    if (strlen(username) == 0) {
        return ERR_OK;
    }

    const struct user_entry *p = find_user(username);
    if (p == NULL) {
        return ERR_VAL;
    }

    if (auth_algo) *auth_algo = p->auth_algo;
    if (auth_key)  MEMCPY(auth_key, p->auth_key, sizeof(p->auth_key));
    if (priv_algo) *priv_algo = p->priv_algo;
    if (priv_key)  MEMCPY(priv_key, p->priv_key, sizeof(p->priv_key));

    return ERR_OK;
}

/* ------------------------------------------------------------------ */
/* Init                                                               */
/* ------------------------------------------------------------------ */

void snmpv3_usm_init(const uint8_t *mac)
{
    /* --- Build engine ID (RFC 3411 §5: enterprise + MAC address format) ---
     *
     * Byte 0-3 : enterprise number (bit 31 = 1, bits 30-0 = enterprise OID)
     * Byte 4   : format = 0x03 (MAC address)
     * Byte 5-10: 6-byte MAC
     */
    uint8_t eid[11];
    eid[0] = (uint8_t)(0x80 | ((SNMP_ENTERPRISE >> 24) & 0x7F));
    eid[1] = (uint8_t)((SNMP_ENTERPRISE >> 16) & 0xFF);
    eid[2] = (uint8_t)((SNMP_ENTERPRISE >>  8) & 0xFF);
    eid[3] = (uint8_t)( SNMP_ENTERPRISE        & 0xFF);
    eid[4] = 0x03;
    memcpy(&eid[5], mac, 6);
    snmpv3_set_engine_id((const char *)eid, sizeof(eid));

    /* --- Engine boots: load from NVS, increment, persist --- */
    uint32_t boots = 1;
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
        uint32_t stored = 0;
        if (nvs_get_u32(h, NVS_KEY_BOOTS, &stored) == ESP_OK) {
            boots = stored + 1;
        }
        nvs_set_u32(h, NVS_KEY_BOOTS, boots);
        nvs_commit(h);
        nvs_close(h);
    }
    s_engine_boots = boots;

    /* --- Derive localized keys (RFC 3414 §2.6) using lwIP helper ---
     *
     * snmpv3_password_to_key_sha():
     *   1. Expands password to 1 MB by repetition
     *   2. SHA-1 hashes → 20-byte master key Ku
     *   3. Localizes: Kl = SHA-1(Ku || engineID || Ku) → 20 bytes
     *
     * auth_key  = full 20 bytes (SHA-1 HMAC key)
     * priv_key  = first 16 bytes used by AES-128 (key derivation
     *             uses a separate passphrase for independent protection)
     */
    const char *eid_ptr;
    u8_t        eid_len;
    snmpv3_get_engine_id(&eid_ptr, &eid_len);

    snmpv3_password_to_key_sha(
        (const u8_t *)SNMPV3_AUTH_PASSPHRASE,
        strlen(SNMPV3_AUTH_PASSPHRASE),
        (const u8_t *)eid_ptr, eid_len,
        s_users[0].auth_key
    );

    snmpv3_password_to_key_sha(
        (const u8_t *)SNMPV3_PRIV_PASSPHRASE,
        strlen(SNMPV3_PRIV_PASSPHRASE),
        (const u8_t *)eid_ptr, eid_len,
        s_users[0].priv_key
    );

    /* --- Start 1-second engine time timer --- */
    esp_timer_create_args_t targs = {
        .callback = engine_time_tick,
        .name     = "snmpv3_time",
    };
    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&targs, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 1000000ULL));

    ESP_LOGI(TAG,
             "SNMPv3 USM ready: user='%s' auth=SHA-1 priv=AES-128 boots=%lu",
             SNMPV3_USERNAME, (unsigned long)boots);
}

#endif /* LWIP_SNMP && LWIP_SNMP_V3 */
