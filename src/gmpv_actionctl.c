/*
 * Copyright (c) 2015-2017 gnome-mpv
 *
 * This file is part of GNOME MPV.
 *
 * GNOME MPV is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNOME MPV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNOME MPV.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <mpv/client.h>
#include <string.h>

#include "gmpv_actionctl.h"
#include "gmpv_file_chooser.h"
#include "gmpv_playlist_widget.h"
#include "gmpv_def.h"
#include "gmpv_mpv.h"
#include "gmpv_mpv_wrapper.h"
#include "gmpv_main_window.h"
#include "gmpv_playlist.h"
#include "gmpv_open_loc_dialog.h"
#include "gmpv_pref_dialog.h"
#include "gmpv_common.h"
#include "gmpv_control_box.h"
#include "gmpv_authors.h"

#if GTK_CHECK_VERSION(3, 20, 0)
#include "gmpv_shortcuts_window.h"
#endif

static void save_playlist(GmpvPlaylist *playlist, GFile *file, GError **error);
static void open_dialog_response_handler(	GtkDialog *dialog,
						gint response_id,
						gpointer data );
static void open_location_dialog_response_handler(	GtkDialog *dialog,
							gint response_id,
							gpointer data );
static void preferences_dialog_response_handler(	GtkDialog *dialog,
							gint response_id,
							gpointer data );
static void show_open_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void show_open_location_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data );
static void toggle_loop_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data );
static void show_shortcuts_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data );
static void toggle_controls_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void toggle_playlist_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void save_playlist_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void remove_selected_playlist_item_handler(	GSimpleAction *action,
							GVariant *param,
							gpointer data );
static void show_preferences_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data );
static void quit_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void set_track_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data );
static void load_track_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void fullscreen_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data );
static void set_video_size_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );
static void show_about_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data );

static void save_playlist(GmpvPlaylist *playlist, GFile *file, GError **error)
{
	gboolean rc = TRUE;
	GOutputStream *dest_stream = NULL;
	GtkTreeModel *model = GTK_TREE_MODEL(gmpv_playlist_get_store(playlist));
	GtkTreeIter iter;

	if(file)
	{
		GFileOutputStream *file_stream;

		file_stream = g_file_replace(	file,
						NULL,
						FALSE,
						G_FILE_CREATE_NONE,
						NULL,
						error );
		dest_stream = G_OUTPUT_STREAM(file_stream);
		rc = gtk_tree_model_get_iter_first(model, &iter);
		rc &= !!dest_stream;
	}

	while(rc)
	{
		gchar *uri;
		gsize written;

		gtk_tree_model_get(model, &iter, PLAYLIST_URI_COLUMN, &uri, -1);

		rc &=	g_output_stream_printf
			(dest_stream, &written, NULL, error, "%s\n", uri);
		rc &= gtk_tree_model_iter_next(model, &iter);
	}

	if(dest_stream)
	{
		g_output_stream_close(dest_stream, NULL, error);
	}
}

static void open_dialog_response_handler(	GtkDialog *dialog,
						gint response_id,
						gpointer data )
{
	GPtrArray *args = data;
	GmpvApplication *app = g_ptr_array_index(args, 0);
	gboolean *append = g_ptr_array_index(args, 1);

	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *file_chooser = GTK_FILE_CHOOSER(dialog);
		GSList *uri_slist = gtk_file_chooser_get_filenames(file_chooser);

		if(uri_slist)
		{
			const gchar **uri_list = gslist_to_array(uri_slist);
			GmpvMpv *mpv = gmpv_application_get_mpv(app);

			gmpv_mpv_load_list(mpv, uri_list, *append, TRUE);
			g_free(uri_list);
		}

		g_slist_free_full(uri_slist, g_free);
	}

	g_free(append);
	g_ptr_array_free(args, TRUE);
	gmpv_file_chooser_destroy(dialog);
}

static void open_location_dialog_response_handler(	GtkDialog *dialog,
							gint response_id,
							gpointer data )
{
	GPtrArray *args = data;
	gboolean *append = g_ptr_array_index(args, 1);

	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		GmpvApplication *app;
		GmpvMpv *mpv;
		const gchar *loc_str;

		app = g_ptr_array_index(args, 0);
		mpv = gmpv_application_get_mpv(app);
		loc_str =	gmpv_open_loc_dialog_get_string
				(GMPV_OPEN_LOC_DIALOG(dialog));

		gmpv_mpv_set_property_flag(mpv, "pause", FALSE);
		gmpv_mpv_load(mpv, loc_str, *append, TRUE);
	}

	g_free(append);
	g_ptr_array_free(args, TRUE);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void preferences_dialog_response_handler(	GtkDialog *dialog,
							gint response_id,
							gpointer data )
{
	if(response_id == GTK_RESPONSE_ACCEPT)
	{
		GmpvApplication *app;
		GmpvMainWindow *wnd;
		GmpvMpv *mpv;
		GSettings *settings;
		gboolean csd_enable;
		gboolean dark_theme_enable;
		gboolean always_floating;

		app = data;
		wnd = gmpv_application_get_main_window(app);
		mpv = gmpv_application_get_mpv(app);
		settings = g_settings_new(CONFIG_ROOT);
		csd_enable = g_settings_get_boolean(settings, "csd-enable");
		dark_theme_enable =	g_settings_get_boolean
					(settings, "dark-theme-enable");
		always_floating =	g_settings_get_boolean
					(settings, "always-use-floating-controls");

		if(gmpv_main_window_get_csd_enabled(wnd) != csd_enable)
		{
			show_message_dialog(	app,
						GTK_MESSAGE_INFO,
						NULL,
						_("Enabling or disabling "
						"client-side decorations "
						"requires restarting %s to "
						"take effect."),
						g_get_application_name());
		}

		g_object_set(	gtk_settings_get_default(),
				"gtk-application-prefer-dark-theme",
				dark_theme_enable,
				NULL );
		g_object_set(	wnd,
				"always-use-floating-controls",
				always_floating,
				NULL );

		gmpv_mpv_reset(mpv);
		gtk_widget_queue_draw(GTK_WIDGET(wnd));
		g_object_unref(settings);
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void show_open_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMainWindow *wnd = NULL;
	GmpvFileChooser *file_chooser = NULL;
	GmpvFileChooser *open_dialog = NULL;
	gboolean *append = g_malloc(sizeof(gboolean));
	GPtrArray *args = g_ptr_array_sized_new(2);

	g_variant_get(param, "b", append);

	wnd = gmpv_application_get_main_window(app);
	open_dialog = gmpv_file_chooser_new(	(*append)?
						_("Add File to Playlist"):
						_("Open File"),
						GTK_WINDOW(wnd),
						GTK_FILE_CHOOSER_ACTION_OPEN );
	file_chooser = GMPV_FILE_CHOOSER(open_dialog);

	g_ptr_array_add(args, app);
	g_ptr_array_add(args, append);

	g_signal_connect(	open_dialog,
				"response",
				G_CALLBACK(open_dialog_response_handler),
				args );

	gmpv_file_chooser_set_default_filters
		(file_chooser, TRUE, TRUE, TRUE, TRUE);

	gmpv_file_chooser_show(open_dialog);
}

static void show_open_location_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data )
{
	GmpvApplication *app = data;
	GmpvMainWindow *wnd = gmpv_application_get_main_window(app);
	GtkWidget *dlg = NULL;
	gboolean *append = g_malloc(sizeof(gboolean));
	GPtrArray *args = g_ptr_array_sized_new(2);

	g_variant_get(param, "b", append);

	dlg = gmpv_open_loc_dialog_new(	GTK_WINDOW(wnd),
					(*append)?
					_("Add Location to Playlist"):
					_("Open Location") );
	g_ptr_array_add(args, app);
	g_ptr_array_add(args, append);

	g_signal_connect
		(	dlg,
			"response",
			G_CALLBACK(open_location_dialog_response_handler),
			args );

	gtk_widget_show_all(dlg);
}

static void toggle_loop_handler(	GSimpleAction *action,
					GVariant *value,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMpv *mpv = gmpv_application_get_mpv(app);
	gboolean loop = g_variant_get_boolean(value);

	g_simple_action_set_state(action, value);
	gmpv_mpv_set_property_string(mpv, "loop", loop?"inf":"no");
}

static void show_shortcuts_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data )
{
#if GTK_CHECK_VERSION(3, 20, 0)
	GmpvApplication *app = data;
	GmpvMainWindow *mwnd = gmpv_application_get_main_window(app);
	GtkWidget *wnd = gmpv_shortcuts_window_new(GTK_WINDOW(mwnd));

	gtk_widget_show_all(wnd);
#endif
}

static void toggle_controls_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMainWindow *wnd = gmpv_application_get_main_window(app);
	GmpvControlBox *ctrl = gmpv_main_window_get_control_box(wnd);
	gboolean visible = gtk_widget_get_visible(GTK_WIDGET(ctrl));

	gtk_widget_set_visible(GTK_WIDGET(ctrl), !visible);
}

static void toggle_playlist_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMainWindow *wnd = gmpv_application_get_main_window(app);
	gboolean visible = gmpv_main_window_get_playlist_visible(wnd);

	gmpv_main_window_set_playlist_visible(wnd, !visible);
}

static void save_playlist_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMpv *mpv;
	GmpvMainWindow *wnd;
	GmpvPlaylist *playlist;
	GFile *dest_file;
	GmpvFileChooser *file_chooser;
	GtkFileChooser *gtk_chooser;
	GError *error;

	mpv = gmpv_application_get_mpv(app);
	wnd = gmpv_application_get_main_window(app);
	playlist = gmpv_mpv_get_playlist(mpv);
	dest_file = NULL;
	file_chooser =	gmpv_file_chooser_new
			(	_("Save Playlist"),
				GTK_WINDOW(wnd),
				GTK_FILE_CHOOSER_ACTION_SAVE );
	gtk_chooser = GTK_FILE_CHOOSER(file_chooser);
	error = NULL;

	gtk_file_chooser_set_current_name(gtk_chooser, "playlist.m3u");

	if(gmpv_file_chooser_run(file_chooser) == GTK_RESPONSE_ACCEPT)
	{
		/* There should be only one file selected. */
		dest_file = gtk_file_chooser_get_file(gtk_chooser);
	}

	gmpv_file_chooser_destroy(file_chooser);

	if(dest_file)
	{
		save_playlist(playlist, dest_file, &error);
		g_object_unref(dest_file);
	}

	if(error)
	{
		show_message_dialog(	app,
					GTK_MESSAGE_ERROR,
					NULL,
					error->message,
					_("Error") );

		g_error_free(error);
	}
}

static void remove_selected_playlist_item_handler(	GSimpleAction *action,
							GVariant *param,
							gpointer data )
{
	GmpvMainWindow *wnd =	gmpv_application_get_main_window
				(GMPV_APPLICATION(data));

	if(gmpv_main_window_get_playlist_visible(wnd))
	{
		GmpvPlaylistWidget *playlist;

		playlist = gmpv_main_window_get_playlist(wnd);

		gmpv_playlist_widget_remove_selected(playlist);
	}
}

static void show_preferences_dialog_handler(	GSimpleAction *action,
						GVariant *param,
						gpointer data )
{
	GmpvApplication *app = data;
	GmpvMainWindow *wnd = gmpv_application_get_main_window(app);
	GtkWidget *pref_dialog = gmpv_pref_dialog_new(GTK_WINDOW(wnd));

	g_signal_connect_after(	pref_dialog,
				"response",
				G_CALLBACK(preferences_dialog_response_handler),
				app );

	gtk_widget_show_all(pref_dialog);
}

static void quit_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	gmpv_application_quit(data);
}

static void set_track_handler(	GSimpleAction *action,
				GVariant *value,
				gpointer data )
{
	GmpvApplication *app = data;
	GmpvMpv *mpv;
	gint64 id;
	gchar *name;
	const gchar *mpv_prop;

	g_object_get(action, "name", &name, NULL);
	g_variant_get(value, "x", &id);
	g_simple_action_set_state(action, value);

	if(g_strcmp0(name, "set-audio-track") == 0)
	{
		mpv_prop = "aid";
	}
	else if(g_strcmp0(name, "set-video-track") == 0)
	{
		mpv_prop = "vid";
	}
	else if(g_strcmp0(name, "set-subtitle-track") == 0)
	{
		mpv_prop = "sid";
	}
	else
	{
		g_assert_not_reached();
	}

	mpv = gmpv_application_get_mpv(app);

	if(id > 0)
	{
		gmpv_mpv_set_property(mpv, mpv_prop, MPV_FORMAT_INT64, &id);
	}
	else
	{
		gmpv_mpv_set_property_string(mpv, mpv_prop, "no");
	}
}

static void load_track_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	GmpvApplication *app = data;
	GmpvMainWindow *wnd;
	GmpvFileChooser *file_chooser;
	const gchar *cmd_name;

	g_variant_get(param, "s", &cmd_name);

	wnd = gmpv_application_get_main_window(app);
	file_chooser =	gmpv_file_chooser_new
			(	_("Load External…"),
				GTK_WINDOW(wnd),
				GTK_FILE_CHOOSER_ACTION_OPEN );

	if(g_strcmp0(cmd_name, "audio-add") == 0)
	{
		gmpv_file_chooser_set_default_filters
			(file_chooser, TRUE, FALSE, FALSE, FALSE);
	}
	else if(g_strcmp0(cmd_name, "sub-add") == 0)
	{
		gmpv_file_chooser_set_default_filters
			(file_chooser, FALSE, FALSE, FALSE, TRUE);
	}
	else
	{
		g_assert_not_reached();
	}

	if(gmpv_file_chooser_run(file_chooser) == GTK_RESPONSE_ACCEPT)
	{
		const gchar *cmd[] = {cmd_name, NULL, NULL};
		GmpvMpv *mpv = gmpv_application_get_mpv(app);
		GtkFileChooser *gtk_chooser = GTK_FILE_CHOOSER(file_chooser);
		GSList *uri_list = gtk_file_chooser_get_filenames(gtk_chooser);

		for(GSList *iter = uri_list; iter; iter = g_slist_next(iter))
		{
			cmd[1] = iter->data;

			gmpv_mpv_command(mpv, cmd);
		}

		g_slist_free_full(uri_list, g_free);
	}

	gmpv_file_chooser_destroy(file_chooser);
}

static void fullscreen_handler(	GSimpleAction *action,
				GVariant *param,
				gpointer data )
{
	GmpvMainWindow *wnd =	gmpv_application_get_main_window
				(GMPV_APPLICATION(data));
	gchar *name;

	g_object_get(action, "name", &name, NULL);

	if(g_strcmp0(name, "toggle-fullscreen") == 0)
	{
		gmpv_main_window_toggle_fullscreen(wnd);
	}
	else if(g_strcmp0(name, "enter-fullscreen") == 0)
	{
		gmpv_main_window_set_fullscreen(wnd, TRUE);
	}
	else if(g_strcmp0(name, "leave-fullscreen") == 0)
	{
		gmpv_main_window_set_fullscreen(wnd, FALSE);
	}

	g_free(name);
}

static void set_video_size_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	gdouble value = g_variant_get_double (param);

	resize_window_to_fit((GmpvApplication *)data, value);
}

static void show_about_dialog_handler(	GSimpleAction *action,
					GVariant *param,
					gpointer data )
{
	GmpvApplication *app = data;
	GmpvMainWindow *wnd = gmpv_application_get_main_window(app);
	const gchar *authors[] = AUTHORS;

	gtk_show_about_dialog(	GTK_WINDOW(wnd),
				"logo-icon-name",
				ICON_NAME,
				"version",
				VERSION,
				"comments",
				_("A GTK frontend for MPV"),
				"website",
				"https://github.com/gnome-mpv/gnome-mpv",
				"license-type",
				GTK_LICENSE_GPL_3_0,
				"copyright",
				"\u00A9 2014-2017 The GNOME MPV authors",
				"authors",
				authors,
				"translator-credits",
				_("translator-credits"),
				NULL );
}

void gmpv_actionctl_map_actions(GmpvApplication *app)
{
	const GActionEntry entries[]
		= {	{.name = "show-open-dialog",
			.activate = show_open_dialog_handler,
			.parameter_type = "b"},
			{.name = "quit",
			.activate = quit_handler},
			{.name = "show-about-dialog",
			.activate = show_about_dialog_handler},
			{.name = "show-preferences-dialog",
			.activate = show_preferences_dialog_handler},
			{.name = "show-open-location-dialog",
			.activate = show_open_location_dialog_handler,
			.parameter_type = "b"},
			{.name = "toggle-loop",
			.state = "false",
			.change_state = toggle_loop_handler},
			{.name = "show-shortcuts-dialog",
			.activate = show_shortcuts_dialog_handler},
			{.name = "toggle-controls",
			.activate = toggle_controls_handler},
			{.name = "toggle-playlist",
			.activate = toggle_playlist_handler},
			{.name = "save-playlist",
			.activate = save_playlist_handler},
			{.name = "remove-selected-playlist-item",
			.activate = remove_selected_playlist_item_handler},
			{.name = "set-audio-track",
			.change_state = set_track_handler,
			.state = "@x 0",
			.parameter_type = "x"},
			{.name = "set-video-track",
			.change_state = set_track_handler,
			.state = "@x 0",
			.parameter_type = "x"},
			{.name = "set-subtitle-track",
			.change_state = set_track_handler,
			.state = "@x 0",
			.parameter_type = "x"},
			{.name = "load-track",
			.activate = load_track_handler,
			.parameter_type = "s"},
			{.name = "toggle-fullscreen",
			.activate = fullscreen_handler},
			{.name = "enter-fullscreen",
			.activate = fullscreen_handler},
			{.name = "leave-fullscreen",
			.activate = fullscreen_handler},
			{.name = "set-video-size",
			.activate = set_video_size_handler,
			.parameter_type = "d"} };

	g_action_map_add_action_entries(	G_ACTION_MAP(app),
						entries,
						G_N_ELEMENTS(entries),
						app );
}
