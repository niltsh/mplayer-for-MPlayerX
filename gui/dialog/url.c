/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "url.h"
#include "dialog.h"
#include "tools.h"
#include "gui/app/app.h"
#include "gui/interface.h"
#include "gui/ui/actions.h"
#include "gui/util/list.h"
#include "gui/util/string.h"

#include "help_mp.h"
#include "stream/stream.h"

GtkWidget *URLDialog = NULL;

static GtkWidget *URLCombo;
static GtkWidget *URLEntry;
static GList *URLComboEntrys = NULL;

static void on_Button_pressed(GtkButton *button, gpointer user_data)
{
    urlItem *item;

    (void)button;

    if ((int)user_data) {
        gchar *str = strdup(gtk_entry_get_text(GTK_ENTRY(URLEntry)));

        if (str) {
            if (!strstr(str, "://")) {
                gchar *tmp;
                tmp = malloc(strlen(str) + 8);
                sprintf(tmp, "http://%s", str);
                free(str);
                str = tmp;
            }

            URLComboEntrys = g_list_prepend(URLComboEntrys, (gchar *)str);

            item      = calloc(1, sizeof(urlItem));
            item->url = gstrdup(str);
            listMgr(URLLIST_ITEM_ADD, item);

            uiSetFile(NULL, str, STREAMTYPE_STREAM);
            guiInfo.NewPlay = GUI_FILE_NEW;
            listMgr(PLAYLIST_DELETE, 0);
            add_to_gui_playlist(str, PLAYLIST_ITEM_APPEND);
            uiEvent(evPlay, 0);
        }
    }

    gtk_widget_destroy(URLDialog);
}

static GtkWidget *CreateURLDialog(void)
{
    GtkWidget *vbox1;
    GtkWidget *hbox1;
    GtkWidget *hbuttonbox1;
    GtkWidget *Ok;
    GtkWidget *Cancel;
    GtkAccelGroup *accel_group;

    accel_group = gtk_accel_group_new();

    URLDialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(URLDialog, "URL");
    gtk_object_set_data(GTK_OBJECT(URLDialog), "URL", URLDialog);
    gtk_widget_set_usize(URLDialog, 384, 70);
    GTK_WIDGET_SET_FLAGS(URLDialog, GTK_CAN_DEFAULT);
    gtk_window_set_title(GTK_WINDOW(URLDialog), MSGTR_Network);
    gtk_window_set_position(GTK_WINDOW(URLDialog), GTK_WIN_POS_CENTER);
    gtk_window_set_policy(GTK_WINDOW(URLDialog), TRUE, TRUE, FALSE);
    gtk_window_set_wmclass(GTK_WINDOW(URLDialog), "Network", "MPlayer");

    gtk_widget_realize(URLDialog);
    gtkAddIcon(URLDialog);

    vbox1 = gtkAddVBox(gtkAddDialogFrame(URLDialog), 0);
    hbox1 = gtkAddHBox(vbox1, 1);
    gtkAddLabel("URL: ", hbox1);

    URLCombo = gtkAddCombo(hbox1);
/*
 * gtk_combo_new();
 * gtk_widget_set_name( URLCombo,"URLCombo" );
 * gtk_widget_show( URLCombo );
 * gtk_box_pack_start( GTK_BOX( hbox1 ),URLCombo,TRUE,TRUE,0 );
 */
    URLEntry = GTK_COMBO(URLCombo)->entry;
    gtk_widget_set_name(URLEntry, "URLEntry");
    gtk_widget_show(URLEntry);

    gtkAddHSeparator(vbox1);

    hbuttonbox1 = gtkAddHButtonBox(vbox1);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox1), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbuttonbox1), 10);

    Ok     = gtkAddButton(MSGTR_Ok, hbuttonbox1);
    Cancel = gtkAddButton(MSGTR_Cancel, hbuttonbox1);

    gtk_widget_add_accelerator(Ok, "clicked", accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(Cancel, "clicked", accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE);

    gtk_signal_connect(GTK_OBJECT(URLDialog), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &URLDialog);
    gtk_signal_connect(GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(on_Button_pressed), (void *)1);
    gtk_signal_connect(GTK_OBJECT(Cancel), "clicked", GTK_SIGNAL_FUNC(on_Button_pressed), NULL);

    gtk_widget_grab_focus(URLEntry);
    gtk_window_add_accel_group(GTK_WINDOW(URLDialog), accel_group);

    return URLDialog;
}

void ShowURLDialog(void)
{
    urlItem *item;

    if (URLDialog)
        gtkRaise(URLDialog);
    else
        URLDialog = CreateURLDialog();

    item = listMgr(URLLIST_GET, 0);

    if (item) {
        g_list_free(URLComboEntrys);
        URLComboEntrys = NULL;

        while (item) {
            URLComboEntrys = g_list_append(URLComboEntrys, (gchar *)item->url);
            item = item->next;
        }
    }

    if (URLComboEntrys) {
        gtk_entry_set_text(GTK_ENTRY(URLEntry), URLComboEntrys->data);
        gtk_combo_set_popdown_strings(GTK_COMBO(URLCombo), URLComboEntrys);
    }

    gtk_widget_show(URLDialog);
}
