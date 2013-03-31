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

#include "config.h"
#include "gui/app/app.h"
#include "help_mp.h"
#include "version.h"

#include "pixmaps/emblem.xpm"
#include "dialog.h"
#include "about.h"
#include "tools.h"

GtkWidget * About = NULL;

static void abWidgetDestroy( GtkButton * button, gpointer user_data )
{
  (void) button;
  (void) user_data;

  gtk_widget_destroy( About );
}

static GtkWidget * CreateAbout( void )
{
  GtkWidget     * vbox;
  GtkWidget     * pixmap1;
  GtkWidget     * scrolledwindow1;
  GtkWidget     * AboutText;
  GtkWidget     * Ok;

#ifdef CONFIG_GTK2
  GtkTextBuffer * AboutTextBuffer;
  GtkTextIter   iter;
#endif /* CONFIG_GTK2 */

  GtkStyle      * pixmapstyle;
  GdkPixmap     * pixmapwid;
  GdkBitmap     * mask;

  GtkAccelGroup * accel_group;

  char title[128];

  accel_group=gtk_accel_group_new();

  About=gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_widget_set_name( About,MSGTR_About );
  gtk_object_set_data( GTK_OBJECT( About ),MSGTR_About,About );
  gtk_widget_set_usize( About,340,415 );
  gtk_window_set_title( GTK_WINDOW( About ),MSGTR_About );
  gtk_window_set_position( GTK_WINDOW( About ),GTK_WIN_POS_CENTER );
  gtk_window_set_policy( GTK_WINDOW( About ),TRUE,FALSE,FALSE );
  gtk_window_set_wmclass( GTK_WINDOW( About ),"About","MPlayer" );

  gtk_widget_realize( About );
  gtkAddIcon( About );

  vbox=gtkAddVBox( gtkAddDialogFrame( About ),0 );

  pixmapstyle=gtk_widget_get_style( About );
  pixmapwid=gdk_pixmap_colormap_create_from_xpm_d( About->window,gdk_colormap_get_system(),&mask,&pixmapstyle->bg[GTK_STATE_NORMAL],(gchar **) emblem_xpm );
  pixmap1=gtk_pixmap_new( pixmapwid,mask );

  gtk_widget_set_name( pixmap1,"pixmap1" );
  gtk_widget_show( pixmap1 );
  gtk_box_pack_start( GTK_BOX( vbox ),pixmap1,FALSE,FALSE,0 );
  gtk_widget_set_usize( pixmap1,-2,174 );

  gtkAddHSeparator( vbox );

  scrolledwindow1=gtk_scrolled_window_new( NULL,NULL );
  gtk_widget_set_name( scrolledwindow1,"scrolledwindow1" );
  gtk_widget_show( scrolledwindow1 );
  gtk_box_pack_start( GTK_BOX( vbox ),scrolledwindow1,TRUE,TRUE,0 );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolledwindow1 ),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC );

#ifdef CONFIG_GTK2
  AboutText = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(AboutText), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(AboutText), FALSE);
  AboutTextBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (AboutText));
  gtk_text_buffer_get_iter_at_offset (AboutTextBuffer, &iter, 0);
#else
  AboutText=gtk_text_new( NULL,NULL );
  gtk_text_set_editable(GTK_TEXT(AboutText), FALSE);
#endif
  gtk_widget_set_name( AboutText,"AboutText" );
  gtk_widget_show( AboutText );
  gtk_container_add( GTK_CONTAINER( scrolledwindow1 ),AboutText );
  snprintf(title, sizeof(title), MP_TITLE, "MPlayer");
#ifdef CONFIG_GTK2
  gtk_text_buffer_insert (AboutTextBuffer, &iter, title, -1);
  gtk_text_buffer_insert (AboutTextBuffer, &iter,
#else
  gtk_text_insert( GTK_TEXT( AboutText ),NULL,NULL,NULL, title, -1 );
  gtk_text_insert( GTK_TEXT( AboutText ),NULL,NULL,NULL,
#endif
  	"\n"
	MSGTR_ABOUT_UHU
	"       (http://www.uhulinux.hu/)\n"
	"\n"
	MSGTR_ABOUT_Contributors
	"\n"
	"     * Andreas Ackermann\n"
	"     * adland\n"
	"     * Eric Anholt\n"
	"     * Folke Ashberg\n"
	"     * Dénes Balatoni\n"
	"     * Zsolt Barat\n"
	"     * Luca Barbato\n"
	"     * Dmitry Baryshkov\n"
	"     * Bertrand Baudet\n"
	"     * Alban Bedel\n"
	"     * Michael Behrisch\n"
	"     * Luchezar Belev\n"
	"     * Gábor Bérczi\n"
	"     * Szabolcs Berecz\n"
	"     * Alex Beregszászi\n"
	"     * Tilmann Bitterberg\n"
	"     * Diego Biurrun\n"
	"     * Marcsu Blomenkamp\n"
	"     * Ingo Brückl\n"
	"     * Andrew Buehler\n"
	"     * Eviv Bulgroz\n"
	"     * Felix Bünemann\n"
	"     * Moritz Bunkus\n"
	"     * Dan Villiom Podlaski Christiansen\n"
	"     * Jeff Clagg\n"
	"     * compn\n"
	"     * Marcus Comstedt\n"
	"     * Kees Cook\n"
	"     * Stephen Davies\n"
	"     * Piero Di Vita\n"
	"     * Tobias Diedrich\n"
	"     * Florian Dietrich\n"
	"     * Jeroen Dobbelaere\n"
	"     * Reimar Döffinger\n"
	"     * Romain Dolbeau\n"
	"     * Ismail Dönmez\n"
	"     * Robert Edele\n"
	"     * Christoph Egger\n"
	"     * Paul Wilhelm Elsinghorst\n"
	"     * Bernd Ernesti\n"
	"     * Salvatore Falco\n"
	"     * Johannes Feigl\n"
	"     * D Richard III Felker\n"
	"     * Tim Ferguson\n"
	"     * Ross Finlayson\n"
	"     * Daniele Forghieri\n"
	"     * Kilian A. Foth\n"
	"     * Fabian Franz\n"
	"     * Martin Gansser\n"
	"     * Árpád Gereöffy\n"
	"     * Matteo Giani\n"
	"     * Sven Goethel\n"
	"     * German Gomez Garcia\n"
	"     * Alexander Gottwald\n"
	"     * Michael Graffam\n"
	"     * Andriy N. Gritsenko\n"
	"     * Rémi Guyomarch\n"
	"     * Jürgen Hammelmann\n"
	"     * Christopher R. Hertel\n"
	"     * Andreas Hess\n"
	"     * Corey Hickey\n"
	"     * Zoltán Hidvégi\n"
	"     * Jens Hoffmann\n"
	"     * David Holm\n"
	"     * Bohdan Horst\n"
	"     * Hampa Hug\n"
	"     * Tomas Hurka\n"
	"     * Sidik Isani\n"
	"     * Panagiotis Issaris\n"
	"     * Aurelien Jacobs\n"
	"     * Reza Jelveh\n"
	"     * Jonas Jermann\n"
	"     * Anders Johansson\n"
	"     * Nicholas Kain\n"
	"     * Filip Kalinski\n"
	"     * Ivan Kalvachev\n"
	"     * Wojtek Kaniewski\n"
	"     * Kim Minh Kaplan\n"
	"     * Samuli Kärkkäinen\n"
	"     * Jürgen Keil\n"
	"     * Robert Kesterson\n"
	"     * Attila Kinali\n"
	"     * Gregory Kovriga\n"
	"     * David Kühling\n"
	"     * Fredrik Kuivinen\n"
	"     * Nick Kurshev\n"
	"     * Brian Kuschak\n"
	"     * Vladimir Kushnir\n"
	"     * Dave Lambley\n"
	"     * Gyula László\n"
	"     * Nicolas Le Gaillart\n"
	"     * Gábor Lénárt\n"
	"     * Colin Leroy\n"
	"     * Oskar Liljeblad\n"
	"     * Sam Lin\n"
	"     * Pierre Lombard\n"
	"     * Puk Madick\n"
	"     * Jindrich Makovicka\n"
	"     * Rudolf Marek\n"
	"     * László Megyer\n"
	"     * Mike Melanson\n"
	"     * Arwed von Merkatz\n"
	"     * Loren Merritt\n"
	"     * Dominik Mierzejewski\n"
	"     * Mihail Milushev\n"
	"     * Nehal Mistry\n"
	"     * András Mohari\n"
	"     * Steven Mueller\n"
	"     * Alexander Neundorf\n"
	"     * Michael Niedermayer\n"
	"     * Fredrik Noring\n"
	"     * Christian Ohm\n"
	"     * Joey Parrish\n"
	"     * Dariusz Pietrzak\n"
	"     * Nicolas Plourde\n"
	"     * Lennart Poettering\n"
	"     * Guillaume Poirier\n"
	"     * Zoltán Ponekker\n"
	"     * Ivo van Poorten\n"
	"     * Lu Ran\n"
	"     * Uwe Reder\n"
	"     * rgselk\n"
	"     * Rune Petersen\n"
	"     * Ville Saari\n"
	"     * Nico Sabbi\n"
	"     * Björn Sandell\n"
	"     * Tilman Sauerbeck\n"
	"     * Frank Scherthan\n"
	"     * Florian Schneider\n"
	"     * Oliver Schoenbrunner\n"
	"     * Oded Shimon\n"
	"     * Peter Simon\n"
	"     * Rik Snel\n"
	"     * Sascha Sommer\n"
	"     * Alexander Strasser\n"
	"     * Kamil Strzelecki\n"
	"     * Jiri Svoboda\n"
	"     * Robert Swain\n"
	"     * Ville Syrjälä\n"
	"     * Gabor Szecsi\n"
	"     * Jason Tackaberry\n"
	"     * Howell Tam\n"
	"     * Adam Tlalka\n"
	"     * Gianluigi Tiesi\n"
	"     * Roberto Togni\n"
	"     * Salvador Eduardo Tropea\n"
	"     * Miklós Vajna\n"
	"     * Reynaldo H. Verdejo Pinochet\n"
	"     * Per Wigren\n"
	"     * Derek J Witt\n"
	"     * Alan Young\n"
	"     * Artur Zaprzala\n"
	"     * Mark Zealey\n"
	"     * Matan Ziv-Av\n"
	"     * Márk Vicián Zoltán\n"
	"\n"
	MSGTR_ABOUT_Codecs_libs_contributions
	"\n"
	"     * Fabrice Bellard\n"
	"     * Vivien Chappelier\n"
	"     * Michael Hipp\n"
	"     * Aaron Holtzman\n"
	"     * Jake Janovetz\n"
	"     * Zdenek Kabelac\n"
	"     * Eugene Kuznetsov\n"
	"     * Michel Lespinasse\n"
	"     * Mark Podlipec\n"
	"     * Damien Vincent\n"
	"\n"
	MSGTR_ABOUT_Translations
	"\n"
	"     * Marcin Biernat\n"
	"     * Marc Fargas\n"
	"     * Jiri Heryan\n"
	"     * Marek Jarycki\n"
	"     * Leszek Kaplita\n"
	"     * Sebastian Krämer\n"
	"     * Juan Martin López\n"
	"     * Piotr Michniewski\n"
	"     * Jakub Misiorny\n"
	"     * Gábor Mizda\n"
	"     * Maciej Paszta\n"
	"     * Łukasz Proszek\n"
	"     * Wacław Schiller\n"
	"     * Andoni Zubimendi\n"
	"\n"
	MSGTR_ABOUT_Skins
	"\n"
	"     * Azrael\n"
	"     * Viktor Bekesi\n"
	"     * Ingo Brückl\n"
	"     * Andrew Carpenter\n"
	"     * Charles Foucault\n"
	"     * Attila Gyimesi\n"
	"     * Alban Hertroys\n"
	"     * Balint Kiss\n"
	"     * Andre Kuehne\n"
	"     * Rüdiger Kuhlmann\n"
	"     * Dan Naumov\n"
	"     * Ryan Northam\n"
	"     * Juan Pablo Oyarzun Arroyo\n"
	"     * DongCheon Park\n"
	"     * Jurgen Pehrson\n"
	"     * Nikola Pizurica\n"
	"     * Oliwier Ptak\n"
	"     * Pasquale Riccio\n"
	"     * Burt.S.\n"
	"     * Jesper Schultz\n"
	"     * Marcin Szumiela\n"
	"     * Massimo Tisi\n"
	"     * Jiri jun. Tyr\n"
	"     * Ognian Vasilev\n"
	"     * Imre Veres\n"
	"     * Radic Vesko\n"
	"     * Balasz Vigvary\n"
	"     * Andrew Weber\n"
	"     * Gary Jr. Whitmore\n"
	"     * Franciszek Wilamowski\n"
	"     * Michael Zeising", -1 );

  gtkAddHSeparator( vbox );
  Ok=gtkAddButton( MSGTR_Ok,gtkAddHButtonBox( vbox ) );

  gtk_signal_connect( GTK_OBJECT( About ),"destroy",GTK_SIGNAL_FUNC( gtk_widget_destroyed ),&About );
  gtk_signal_connect_object( GTK_OBJECT( Ok ),"clicked",GTK_SIGNAL_FUNC( abWidgetDestroy ),NULL );

  gtk_widget_add_accelerator( Ok,"clicked",accel_group,GDK_Escape,0,GTK_ACCEL_VISIBLE );
  gtk_widget_add_accelerator( Ok,"clicked",accel_group,GDK_Return,0,GTK_ACCEL_VISIBLE );
  gtk_window_add_accel_group( GTK_WINDOW( About ),accel_group );

  return About;
}

void ShowAbout( void )
{
 if ( About ) gtkRaise( About );
   else About=CreateAbout();
 gtk_widget_show( About );
}
