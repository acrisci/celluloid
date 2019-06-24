/*
 * Copyright (c) 2017-2019 gnome-mpv
 *
 * This file is part of Celluloid.
 *
 * Celluloid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Celluloid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Celluloid.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <glib.h>

#include "celluloid-model.h"
#include "celluloid-view.h"

G_BEGIN_DECLS

#define CELLULOID_TYPE_CONTROLLER (celluloid_controller_get_type())

G_DECLARE_FINAL_TYPE(CelluloidController, celluloid_controller, CELLULOID, CONTROLLER, GObject)

CelluloidController *
celluloid_controller_new(CelluloidApplication *app);

void
celluloid_controller_quit(CelluloidController *controller);

void
celluloid_controller_autofit(	CelluloidController *controller,
				gdouble multiplier );

void
celluloid_controller_present(CelluloidController *controller);

void
celluloid_controller_open(	CelluloidController *controller,
				const gchar *uri,
				gboolean append );

CelluloidView *
celluloid_controller_get_view(CelluloidController *controller);

CelluloidModel *
celluloid_controller_get_model(CelluloidController *controller);

G_END_DECLS

#endif
