#include <gtktopology.h>

G_DEFINE_TYPE (GtkTopology, gtk_topology, GTK_TYPE_DRAWING_AREA);

static void gtk_topology_class_init(GtkTopologyClass *class)
{

}

static void gtk_topology_init(GtkTopology *topology)
{

}

GtkWidget* gtk_topology_new(void)
{
	return g_object_new(GTK_TYPE_TOPOLOGY, NULL);
}

static gboolean gtk_topology_expose(GtkWidget *clock, GdkEvent *event)
{
	return FALSE;
}
