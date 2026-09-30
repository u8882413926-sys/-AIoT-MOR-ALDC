/*
 * network_client.h
 * ===========================================================================
 * TCP Client — sends AIoT-MOR-ALDC results to the Windows laptop server
 * Uses Azure RTOS NetX Duo (best choice for RA8P1 FSP v6.5+ Ethernet)
 * ===========================================================================
 */

#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "ai_pipeline.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ── Network configuration ────────────────────────────────────────────────
 * Board static IP  : 192.168.10.200
 * Laptop static IP : 192.168.10.100  ← Set this on Windows Ethernet adapter
 * Server port      : 8888
 * ─────────────────────────────────────────────────────────────────────── */
#define NET_BOARD_IP_0      192
#define NET_BOARD_IP_1      168
#define NET_BOARD_IP_2       10
#define NET_BOARD_IP_3      200

#define NET_SERVER_IP_0     192
#define NET_SERVER_IP_1     168
#define NET_SERVER_IP_2      10
#define NET_SERVER_IP_3     100

#define NET_SERVER_PORT     8888
#define NET_SUBNET_MASK     0xFFFFFF00UL  /* 255.255.255.0 */

/* ── Maximum JSON string length ──────────────────────────────────────── */
#define NET_JSON_BUF_SIZE   512

/* ── Packet structure sent over TCP ───────────────────────────────────── */
typedef struct {
    uint32_t        packet_id;
    float           temp_c;
    float           humidity;
    int             signal_class;
    const char     *signal_class_name;
    bool            is_anomaly;
    bool            do_compress;
    float           cr;
    uint32_t        orig_bytes;
    uint32_t        comp_bytes;
    uint32_t        time_ms;
    float           features[8];       /* AI_NUM_FEATURES extracted features */
    struct tm       timestamp;         /* RTC wall-clock time                */
    uint32_t        uptime_sec;        /* Seconds since boot                */
    /* Sensor source flags — set by sensor_source_read() */
    bool            src_sensor;        /* true: data came from real DHT11    */
    bool            src_simulated;     /* true: data is pseudo-random        */
    bool            src_injected;      /* true: data injected via RTT/laptop */
} net_packet_t;

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initialise the NetX Duo stack and connect to the server.
 *         Call once at startup from the network task.
 * @return true on successful connection, false on error.
 */
bool network_client_init(void);

/**
 * @brief  Send one data packet as a JSON string over the existing TCP connection.
 *         If the connection was lost, it will attempt one reconnect.
 * @return true on success.
 */
bool network_client_send(const net_packet_t *pkt);

/**
 * @brief  Gracefully close the TCP connection.
 */
void network_client_close(void);

/**
 * @brief  Check if the client is currently connected.
 * @return true if connected.
 */
bool network_client_is_connected(void);

#endif /* NETWORK_CLIENT_H */
