#include <gtktopology.h>

G_DEFINE_TYPE (GtkTopology, gtk_topology, GTK_TYPE_DRAWING_AREA);

static gboolean gtk_topology_expose (GtkWidget *topology, GdkEventExpose *event);

static void gtk_topology_class_init(GtkTopologyClass *class)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS(class);

	widget_class->expose_event = gtk_topology_expose;
}

static void gtk_topology_init(GtkTopology *topology)
{
	INIT_LIST_HEAD(&topology->devices);
}

GtkWidget* gtk_topology_new(void)
{
	return g_object_new(GTK_TYPE_TOPOLOGY, NULL);
}

static void draw(GtkWidget* topology, cairo_t *cairo)
{
	int width = topology->allocation.width;
	int height = topology->allocation.height;
	cairo_set_source_rgb(cairo, 1, 1, 1);
	cairo_rectangle(cairo, 0, 0, width, height);
	cairo_fill(cairo);
}

static gboolean gtk_topology_expose(GtkWidget *topology, GdkEventExpose *event)
{
	cairo_t *cairo;

	cairo =  gdk_cairo_create(topology->window);

	draw(topology, cairo);

	cairo_destroy(cairo);
	
	return FALSE;
}

GtkTopologyDevice* gtk_topology_new_router()
{
	return NULL;
}

GtkTopologyDevice* gtk_topology_new_switch()
{
	return NULL;
}

void gtk_topology_add_device(GtkTopology *topology, GtkTopologyDevice *device)
{
	INIT_LIST_HEAD(&(device->list));
	list_add(&device->list, &topology->devices);
}
