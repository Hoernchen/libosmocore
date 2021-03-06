/*! \file frame_relay.h */

/* (C) 2020 Harald Welte <laforge@gnumonks.org>
 * (C) 2020 sysmocom - s.f.m.c. GmbH
 * Author: Alexander Couzens <lynxis@fe80.eu>
 *
 * All Rights Reserved
 *
 * SPDX-License-Identifier: GPL-2.0+
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <osmocom/core/linuxlist.h>
#include <osmocom/core/timer.h>
#include <osmocom/core/utils.h>

#include <stdint.h>

struct osmo_tdef;
struct msgb;

enum osmo_fr_role {
	FR_ROLE_USER_EQUIPMENT,
	FR_ROLE_NETWORK_EQUIPMENT,
};

extern const struct value_string osmo_fr_role_names[];

static inline const char *osmo_fr_role_str(enum osmo_fr_role role) {
	return get_value_string(osmo_fr_role_names, role);
}

struct osmo_fr_network {
	struct llist_head links;

	unsigned int n391; 		/* full status polling counter */
	unsigned int n392;		/* error threshold */
	unsigned int n393;		/* monitored events count */

	struct osmo_tdef *T_defs;	/* T391, T392 */
};

struct osmo_fr_dlc;

/* Frame Relay Link */
struct osmo_fr_link {
	/* list in osmo_fr_network.links */
	struct llist_head list;
	struct osmo_fr_network *net;
	enum osmo_fr_role role;
	/* human-readable name */
	const char *name;

	/* value of the last received send sequence number field in the
	 * link integrity verification information element */
	uint8_t last_rx_seq;

	/* value of the send sequence number field of the last link
	 * integrity verification information element sent */
	uint8_t last_tx_seq;

	struct osmo_timer_list t391;
	struct osmo_timer_list t392;

	unsigned int polling_count;
	unsigned int err_count;
	unsigned int succeed;
	/* the type of the last status enquiry */
	uint8_t expected_rep;
	bool state;

	/* list of data link connections at this link */
	struct llist_head dlc_list;

	int (*unknown_dlc_rx_cb)(void *cb_data, struct msgb *msg);
	void *unknown_dlc_rx_cb_data;

	int (*tx_cb)(void *data, struct msgb *msg);
	void *tx_cb_data;
};

/* Frame Relay Data Link Connection */
struct osmo_fr_dlc {
	/* entry in fr_link.dlc_list */
	struct llist_head list;
	struct osmo_fr_link *link;

	uint16_t dlci;

	/* is this DLC marked active for traffic? */
	bool active;
	/* was this DLC newly added? */
	bool add;
	/* is this DLC about to be destroyed */
	bool del;

	/* The local state needs to be transferred to the USER;
	 * NET must wait until USER confirms it implicitly by a seq number check */
	bool state_send;

	int (*rx_cb)(void *cb_data, struct msgb *msg);
	void *rx_cb_data;
};

/* allocate a frame relay network */
struct osmo_fr_network *osmo_fr_network_alloc(void *ctx);

/* allocate a frame relay link in a given network */
struct osmo_fr_link *osmo_fr_link_alloc(struct osmo_fr_network *net, enum osmo_fr_role role, const char *name);

/* free a frame link in a given network */
void osmo_fr_link_free(struct osmo_fr_link *link);

/* allocate a data link connectoin on a given framerelay link */
struct osmo_fr_dlc *osmo_fr_dlc_alloc(struct osmo_fr_link *link, uint16_t dlci);
void osmo_fr_dlc_free(struct osmo_fr_dlc *dlc);

struct osmo_fr_dlc *osmo_fr_dlc_by_dlci(struct osmo_fr_link *link, uint16_t dlci);

int osmo_fr_rx(struct msgb *msg);
int osmo_fr_tx_dlc(struct msgb *msg);
