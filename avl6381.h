/*

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Availink avl6381 demod driver
 *
 * Copyright (C) 2024 Xiaodong Ni <nxiaodong520@gmail.com>
 */

#ifndef AVL6381_H
#define AVL6381_H

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
#include <dvb_frontend.h>
#else
#include <media/dvb_frontend.h>
#endif

struct avl6381_config {
	void		*i2c_adapter;  // i2c adapter
	u8		demod_address; // demodulator i2c address
	u8		tuner_address; // tuner i2c address
	int (*tuner_select_input) (struct dvb_frontend *fe, enum fe_delivery_system delivery_system);
};

extern struct dvb_frontend *avl6381_attach(struct avl6381_config *config, struct i2c_adapter *i2c);

#endif /* AVL6381_H */
